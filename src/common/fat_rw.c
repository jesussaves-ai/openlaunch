/*
 * src/common/fat_rw.c
 *
 * Minimal FAT16/FAT32 root-file writer. See include/common/fat_rw.h.
 *
 * Bounded and fail-closed: every geometry value is validated, every sector
 * read/write is checked, cluster chains are walked with a hop cap, and any
 * failure in the middle of a create either leaves the file deleted or frees
 * the clusters it allocated. It reuses the read callback of the mounted
 * fat_volume_t for read-modify-write of directory/FAT sectors.
 */
#include "common/fat_rw.h"
#include "common/mem.h"

#define FAT16_EOC      0xFFFFu
#define FAT32_EOC      0x0FFFFFFFu
#define FAT32_MASK     0x0FFFFFFFu

/* ---- Low-level sector helpers ----------------------------------------- */

static bool rw_read(fat_writer_t *w, u32 lba)
{
    if (!w->vol->read(w->vol->ctx, lba, w->buf)) {
        w->vol->io_error = true;
        return false;
    }
    return true;
}

static bool rw_write(fat_writer_t *w, u32 lba)
{
    return w->write(w->ctx, lba, w->buf);
}

static u32 rw_eoc(const fat_volume_t *vol)
{
    return (vol->type == FAT_TYPE_FAT32) ? FAT32_EOC : FAT16_EOC;
}

static u32 rw_entry_bytes(const fat_volume_t *vol)
{
    return (vol->type == FAT_TYPE_FAT32) ? 4u : 2u;
}

static u32 rw_cluster_lba(const fat_volume_t *vol, u32 cluster)
{
    return vol->data_start_lba +
           (cluster - 2u) * (u32)vol->sectors_per_cluster;
}

static u32 rw_cluster_bytes(const fat_volume_t *vol)
{
    return (u32)vol->sectors_per_cluster * FAT_SECTOR_SIZE;
}

/* Read one FAT entry (masked for FAT32). */
static fat_write_status_t rw_read_entry(fat_writer_t *w, u32 cluster, u32 *out)
{
    u32 entry_size = rw_entry_bytes(w->vol);
    u32 off = cluster * entry_size;
    u32 lba = w->vol->fat_start_lba + off / FAT_SECTOR_SIZE;
    u32 so = off % FAT_SECTOR_SIZE;

    if (cluster < 2u || cluster > w->vol->max_cluster) {
        return FAT_WR_ERR_BAD_CLUSTER;
    }
    if (!rw_read(w, lba)) {
        return FAT_WR_ERR_IO;
    }
    if (w->vol->type == FAT_TYPE_FAT32) {
        *out = dsi_read_le32(w->buf + so) & FAT32_MASK;
    } else {
        *out = dsi_read_le16(w->buf + so);
    }
    return FAT_WR_OK;
}

/* Write one FAT entry to every FAT copy on the volume. */
static fat_write_status_t rw_write_entry(fat_writer_t *w, u32 cluster, u32 value)
{
    u32 entry_size = rw_entry_bytes(w->vol);
    u32 off = cluster * entry_size;
    u32 base_lba = w->vol->fat_start_lba + off / FAT_SECTOR_SIZE;
    u32 so = off % FAT_SECTOR_SIZE;
    u8 i;

    if (cluster < 2u || cluster > w->vol->max_cluster) {
        return FAT_WR_ERR_BAD_CLUSTER;
    }

    for (i = 0u; i < w->vol->num_fats; i++) {
        u32 lba = base_lba + (u32)i * w->vol->fat_size;

        if (!rw_read(w, lba)) {
            return FAT_WR_ERR_IO;
        }
        if (w->vol->type == FAT_TYPE_FAT32) {
            dsi_write_le32(w->buf + so, value & FAT32_MASK);
        } else {
            w->buf[so] = (u8)(value & 0xFFu);
            w->buf[so + 1u] = (u8)((value >> 8) & 0xFFu);
        }
        if (!rw_write(w, lba)) {
            return FAT_WR_ERR_IO;
        }
    }
    return FAT_WR_OK;
}

/* Find and reserve a free data cluster. */
static fat_write_status_t rw_alloc_cluster(fat_writer_t *w, u32 *out)
{
    u32 c;

    for (c = w->alloc_hint; c <= w->vol->max_cluster; c++) {
        u32 val;
        fat_write_status_t st = rw_read_entry(w, c, &val);

        if (st != FAT_WR_OK) {
            return st;
        }
        if (val == 0u) {
            st = rw_write_entry(w, c, rw_eoc(w->vol));
            if (st != FAT_WR_OK) {
                return st;
            }
            w->alloc_hint = c + 1u;
            *out = c;
            return FAT_WR_OK;
        }
    }
    return FAT_WR_ERR_NO_SPACE;
}

/* Free a cluster chain starting at `first`. */
static void rw_free_chain(fat_writer_t *w, u32 first)
{
    u32 c = first;
    u32 hops = 0u;
    u32 max_hops = w->vol->cluster_count + 2u;

    while (c >= 2u && c <= w->vol->max_cluster && hops <= max_hops) {
        u32 next = 0u;

        if (rw_read_entry(w, c, &next) != FAT_WR_OK) {
            return;
        }
        if (rw_write_entry(w, c, 0u) != FAT_WR_OK) {
            return;
        }
        if (w->vol->type == FAT_TYPE_FAT32) {
            if (next >= 0x0FFFFFF8u || next < 2u) {
                return;
            }
        } else {
            if (next >= FAT16_EOC || next < 2u) {
                return;
            }
        }
        c = next;
        hops++;
    }
}

/* ---- 8.3 name handling ------------------------------------------------ */

static bool name_char_ok(char c)
{
    if (c >= 'A' && c <= 'Z') { return true; }
    if (c >= 'a' && c <= 'z') { return true; }
    if (c >= '0' && c <= '9') { return true; }
    switch (c) {
    case '_': case '-': case '~': case '!': case '#': case '$':
    case '%': case '&': case '(': case ')': case '@': case '^':
    case '\'': case '{': case '}': case '`':
        return true;
    default:
        return false;
    }
}

static char name_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - ('a' - 'A')) : c;
}

/* Encode "NAME.EXT" (either case) into the 11-byte on-disk field. */
static bool name_to_83(const char *name, u8 out[11])
{
    u32 i;
    u32 n = 0u;
    u32 name_len = 0u;
    const char *dot = NULL;
    const char *p;

    if (name == NULL) {
        return false;
    }
    for (p = name; *p != '\0'; p++) {
        if (*p == '.') {
            if (dot != NULL) {
                return false;   /* more than one dot */
            }
            dot = p;
        }
    }

    for (i = 0u; i < 11u; i++) {
        out[i] = ' ';
    }

    for (p = name; *p != '\0' && *p != '.'; p++) {
        if (n >= 8u || !name_char_ok(*p)) {
            return false;
        }
        out[n++] = (u8)name_upper(*p);
        name_len++;
    }
    if (name_len == 0u) {
        return false;
    }

    if (dot != NULL) {
        u32 e = 0u;

        for (p = dot + 1; *p != '\0'; p++) {
            if (e >= 3u || !name_char_ok(*p)) {
                return false;
            }
            out[8u + e] = (u8)name_upper(*p);
            e++;
        }
    }
    return true;
}

static bool entry_name_matches(const u8 *e, const u8 name11[11])
{
    u32 i;

    for (i = 0u; i < 11u; i++) {
        if (e[i] != name11[i]) {
            return false;
        }
    }
    return true;
}

/* ---- Root directory slot search --------------------------------------- */

/*
 * Scan the root directory for a free slot, reporting a duplicate name as
 * EXISTS. `*out_lba`/`*out_off` receive the first deleted slot if one exists,
 * otherwise the first end-of-directory (0x00) slot.
 */
static fat_write_status_t find_root_slot(fat_writer_t *w, const u8 name11[11],
                                         u32 *out_lba, u32 *out_off)
{
    fat_volume_t *vol = w->vol;
    bool have_deleted = false;
    u32 del_lba = 0u;
    u32 del_off = 0u;
    u32 cluster;
    u32 hops;
    u32 max_hops;

    if (vol->type == FAT_TYPE_FAT32) {
        cluster = vol->root_cluster;
        if (cluster < 2u || cluster > vol->max_cluster) {
            return FAT_WR_ERR_BAD_CLUSTER;
        }
        max_hops = vol->cluster_count + 2u;
        hops = 0u;
    } else {
        cluster = 0u;   /* marker: fixed root handled below */
        max_hops = 0u;
        hops = 0u;
    }

    for (;;) {
        u32 s;
        u32 sectors = (vol->type == FAT_TYPE_FAT32)
                          ? (u32)vol->sectors_per_cluster
                          : vol->root_sectors;
        u32 base = (vol->type == FAT_TYPE_FAT32)
                       ? rw_cluster_lba(vol, cluster)
                       : vol->root_start_lba;

        for (s = 0u; s < sectors; s++) {
            u32 idx;

            if (!rw_read(w, base + s)) {
                return FAT_WR_ERR_IO;
            }
            for (idx = 0u; idx < FAT_DIR_ENTRIES_PER_SECTOR; idx++) {
                const u8 *e = w->buf + idx * FAT_DIR_ENTRY_SIZE;
                u8 attr;

                if (e[0] == 0x00u) {
                    if (have_deleted) {
                        *out_lba = del_lba;
                        *out_off = del_off;
                    } else {
                        *out_lba = base + s;
                        *out_off = idx * FAT_DIR_ENTRY_SIZE;
                    }
                    return FAT_WR_OK;
                }
                if (e[0] == 0xE5u) {
                    if (!have_deleted) {
                        have_deleted = true;
                        del_lba = base + s;
                        del_off = idx * FAT_DIR_ENTRY_SIZE;
                    }
                    continue;
                }
                attr = e[11];
                if ((attr & FAT_ATTR_LFN) == FAT_ATTR_LFN) {
                    continue;
                }
                if ((attr & FAT_ATTR_VOLUME_ID) != 0u) {
                    continue;
                }
                if (entry_name_matches(e, name11)) {
                    return FAT_WR_ERR_EXISTS;
                }
            }
        }

        if (vol->type != FAT_TYPE_FAT32) {
            break;   /* fixed root: single pass */
        }

        {
            u32 next = fat_get_next_cluster(vol, cluster);

            if (vol->io_error) {
                return FAT_WR_ERR_IO;
            }
            if (vol->type == FAT_TYPE_FAT32) {
                if (next >= 0x0FFFFFF8u || next < 2u ||
                    next > vol->max_cluster) {
                    break;   /* end of the root chain */
                }
            }
            if (next == cluster || ++hops > max_hops) {
                return FAT_WR_ERR_BAD_CLUSTER;
            }
            cluster = next;
        }
    }

    if (have_deleted) {
        *out_lba = del_lba;
        *out_off = del_off;
        return FAT_WR_OK;
    }
    return FAT_WR_ERR_NO_SPACE;
}

/* Rewrite the file's directory entry with its current start cluster + size. */
static fat_write_status_t update_dir_entry(fat_writer_t *w)
{
    u8 *e;

    if (!rw_read(w, w->dir_lba)) {
        return FAT_WR_ERR_IO;
    }
    e = w->buf + w->dir_off;
    /* FAT32 splits the start cluster across +20 (high) and +26 (low). */
    e[20] = (u8)((w->first_cluster >> 16) & 0xFFu);
    e[21] = (u8)((w->first_cluster >> 24) & 0xFFu);
    e[26] = (u8)(w->first_cluster & 0xFFu);
    e[27] = (u8)((w->first_cluster >> 8) & 0xFFu);
    dsi_write_le32(e + 28u, w->total_bytes);
    if (!rw_write(w, w->dir_lba)) {
        return FAT_WR_ERR_IO;
    }
    return FAT_WR_OK;
}

/* ---- Public API ------------------------------------------------------- */

fat_write_status_t fat_create_file(fat_volume_t *vol,
                                   fat_write_sector_fn write, void *ctx,
                                   const char *name, fat_writer_t *w)
{
    u8 name11[11];
    u32 slot_lba = 0u;
    u32 slot_off = 0u;
    fat_write_status_t st;

    if (vol == NULL || w == NULL || write == NULL || name == NULL ||
        vol->type == FAT_TYPE_NONE || vol->max_cluster < 2u) {
        return FAT_WR_ERR_ARG;
    }
    if (!name_to_83(name, name11)) {
        return FAT_WR_ERR_NAME;
    }

    dsi_memset(w, 0, sizeof(*w));
    w->vol = vol;
    w->write = write;
    w->ctx = ctx;
    w->alloc_hint = 2u;

    st = find_root_slot(w, name11, &slot_lba, &slot_off);
    if (st != FAT_WR_OK) {
        return st;
    }

    /* Write the initial (empty) directory entry. */
    if (!rw_read(w, slot_lba)) {
        return FAT_WR_ERR_IO;
    }
    dsi_memset(w->buf + slot_off, 0, FAT_DIR_ENTRY_SIZE);
    dsi_memcpy(w->buf + slot_off, name11, 11u);
    w->buf[slot_off + 11u] = FAT_ATTR_ARCHIVE;
    if (!rw_write(w, slot_lba)) {
        return FAT_WR_ERR_IO;
    }

    w->active = true;
    w->dir_lba = slot_lba;
    w->dir_off = slot_off;
    w->first_cluster = 0u;
    w->cur_cluster = 0u;
    w->cluster_offset = 0u;
    w->total_bytes = 0u;
    return FAT_WR_OK;
}

fat_write_status_t fat_writer_write(fat_writer_t *w, const u8 *data, u32 len)
{
    u32 cluster_bytes;

    if (w == NULL || !w->active) {
        return FAT_WR_ERR_ARG;
    }
    if (data == NULL && len != 0u) {
        return FAT_WR_ERR_ARG;
    }
    if (len == 0u) {
        return FAT_WR_OK;
    }

    cluster_bytes = rw_cluster_bytes(w->vol);
    if (cluster_bytes == 0u) {
        return FAT_WR_ERR_BAD_CLUSTER;
    }

    while (len > 0u) {
        u32 frame_off;
        u32 avail;
        u32 chunk;

        /* Allocate the first cluster lazily, or link the next one. */
        if (w->cur_cluster == 0u || w->cluster_offset >= cluster_bytes) {
            u32 next;

            {
                fat_write_status_t st = rw_alloc_cluster(w, &next);

                if (st != FAT_WR_OK) {
                    return st;
                }
            }
            if (w->cur_cluster == 0u) {
                w->first_cluster = next;
                w->cur_cluster = next;
                w->cluster_offset = 0u;
                {
                    fat_write_status_t st = update_dir_entry(w);

                    if (st != FAT_WR_OK) {
                        return st;
                    }
                }
            } else {
                fat_write_status_t st = rw_write_entry(w, w->cur_cluster, next);

                if (st != FAT_WR_OK) {
                    return st;
                }
                w->cur_cluster = next;
                w->cluster_offset = 0u;
            }
        }

        avail = cluster_bytes - w->cluster_offset;
        chunk = (len < avail) ? len : avail;
        frame_off = 0u;

        while (frame_off < chunk) {
            u32 pos = w->cluster_offset + frame_off;
            u32 lba = rw_cluster_lba(w->vol, w->cur_cluster) +
                      pos / FAT_SECTOR_SIZE;
            u32 so = pos % FAT_SECTOR_SIZE;
            u32 n = FAT_SECTOR_SIZE - so;

            if (n > chunk - frame_off) {
                n = chunk - frame_off;
            }
            if (so == 0u && n == FAT_SECTOR_SIZE) {
                dsi_memcpy(w->buf, data + frame_off, FAT_SECTOR_SIZE);
            } else {
                if (!rw_read(w, lba)) {
                    return FAT_WR_ERR_IO;
                }
                dsi_memcpy(w->buf + so, data + frame_off, n);
            }
            if (!rw_write(w, lba)) {
                return FAT_WR_ERR_IO;
            }
            frame_off += n;
        }

        w->cluster_offset += chunk;
        w->total_bytes += chunk;
        data += chunk;
        len -= chunk;
    }

    return FAT_WR_OK;
}

fat_write_status_t fat_writer_end(fat_writer_t *w, u32 *out_size)
{
    fat_write_status_t st;

    if (w == NULL || !w->active) {
        return FAT_WR_ERR_ARG;
    }
    st = update_dir_entry(w);
    if (st != FAT_WR_OK) {
        return st;
    }
    w->active = false;
    if (out_size != NULL) {
        *out_size = w->total_bytes;
    }
    return FAT_WR_OK;
}

void fat_writer_abort(fat_writer_t *w)
{
    if (w == NULL || !w->active) {
        return;
    }
    if (w->first_cluster >= 2u) {
        rw_free_chain(w, w->first_cluster);
    }
    if (rw_read(w, w->dir_lba)) {
        w->buf[w->dir_off] = 0xE5u;   /* mark the entry deleted */
        (void)rw_write(w, w->dir_lba);
    }
    w->active = false;
}

const char *fat_write_status_text(fat_write_status_t status)
{
    switch (status) {
    case FAT_WR_OK:              return "OK";
    case FAT_WR_ERR_ARG:         return "BAD ARGUMENT";
    case FAT_WR_ERR_NAME:        return "BAD 8.3 NAME";
    case FAT_WR_ERR_IO:          return "WRITE ERROR";
    case FAT_WR_ERR_EXISTS:      return "FILE EXISTS";
    case FAT_WR_ERR_NO_SPACE:    return "NO FREE SPACE";
    case FAT_WR_ERR_BAD_CLUSTER: return "BAD CLUSTER";
    default:                     return "UNKNOWN";
    }
}
