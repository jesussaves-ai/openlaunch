/*
 * src/common/fat_ro.c
 *
 * Zero-allocation, bounds-checked, read-only FAT16 / FAT32 parser.
 * See include/common/fat_ro.h for the contract.
 *
 * All on-disk values are read little-endian through the unaligned accessors in
 * common/mem.h and validated before use. The only scratch space is the single
 * sector buffer inside fat_volume_t.
 */
#include "common/fat_ro.h"
#include "common/mem.h"

/* FAT entry special values. */
#define FAT16_EOC_MIN     0xFFF8u
#define FAT16_BAD         0xFFF7u
#define FAT32_EOC_MIN     0x0FFFFFF8u
#define FAT32_BAD         0x0FFFFFF7u
#define FAT32_MASK        0x0FFFFFFFu

#define FAT_INVALID_CLUSTER 0xFFFFFFFFu

/* Partition-entry offsets within an MBR sector. */
#define MBR_PART_OFF      446u
#define MBR_PART_STRIDE   16u
#define MBR_PART_COUNT    4u
#define MBR_SIG_OFF       510u
#define MBR_SIG_VALUE     0xAA55u
#define MBR_PART_TYPE_OFF 4u
#define MBR_PART_LBA_OFF  8u
#define MBR_PART_CNT_OFF  12u

/* ---- Small helpers ----------------------------------------------------- */

static bool vol_read_sector(fat_volume_t *vol, u32 lba, u8 *buf)
{
    if (!vol->read(vol->ctx, lba, buf)) {
        vol->io_error = true;
        return false;
    }
    return true;
}

static bool part_type_is_fat(u8 type)
{
    return type == FAT_PART_TYPE_FAT16_SMALL ||
           type == FAT_PART_TYPE_FAT16 ||
           type == FAT_PART_TYPE_FAT32 ||
           type == FAT_PART_TYPE_FAT32_LBA ||
           type == FAT_PART_TYPE_FAT16_LBA;
}

static bool cluster_in_use(const fat_volume_t *vol, u32 cluster)
{
    return cluster >= 2u && cluster <= vol->max_cluster;
}

static bool cluster_is_bad(const fat_volume_t *vol, u32 cluster)
{
    if (vol->type == FAT_TYPE_FAT32) {
        return cluster == FAT32_BAD;
    }
    return cluster == FAT16_BAD;
}

static u32 cluster_lba(const fat_volume_t *vol, u32 cluster)
{
    return vol->data_start_lba +
           (cluster - 2u) * (u32)vol->sectors_per_cluster;
}

static bool byte_is_space_or_nul(u8 c)
{
    return c == ' ' || c == 0x00u;
}

/* Case-insensitive compare of a 3-byte FAT extension field against "NDS"/etc. */
static bool ext_equals(const u8 *ext, const char *want)
{
    u32 i;

    for (i = 0u; i < 3u; i++) {
        u8 c = ext[i];

        if (c >= 'a' && c <= 'z') {
            c = (u8)(c - ('a' - 'A'));
        }
        if (c != (u8)want[i]) {
            return false;
        }
    }
    return true;
}

static bool short_name_is_nds(const u8 *e)
{
    return ext_equals(e + 8u, "NDS") || ext_equals(e + 8u, "SRL");
}

/*
 * Expand an 8.3 directory-entry name into "NAME.EXT". Trailing spaces are
 * trimmed; the output is always NUL-terminated within FAT_NAME_MAX bytes.
 */
static void parse_short_name(const u8 *e, char out[FAT_NAME_MAX])
{
    u32 i;
    u32 n = 0u;

    for (i = 0u; i < 8u && n < FAT_NAME_MAX - 1u; i++) {
        u8 c = e[i];

        if (c == ' ') {
            break;
        }
        out[n++] = (char)c;
    }

    if (!byte_is_space_or_nul(e[8]) && n < FAT_NAME_MAX - 1u) {
        out[n++] = '.';
        for (i = 8u; i < 11u && n < FAT_NAME_MAX - 1u; i++) {
            u8 c = e[i];

            if (c == ' ') {
                break;
            }
            out[n++] = (char)c;
        }
    }

    out[n] = '\0';
}

/* ---- BPB parsing ------------------------------------------------------- */

/*
 * Parse the BIOS Parameter Block at absolute sector `base` and fill in the
 * volume geometry. Returns false if the sector is not a supported FAT volume.
 */
static bool fat_parse_bpb(fat_volume_t *vol, u32 base)
{
    const u8 *b;
    u16 bytes_per_sector;
    u16 reserved;
    u16 root_entries;
    u16 total16;
    u16 fat_size16;
    u32 total32;
    u32 fat_size32;
    u32 root_cluster;
    u32 total;
    u32 fat_size;
    u32 root_sectors;
    u32 overhead;
    u32 cluster_count;

    if (!vol_read_sector(vol, base, vol->sector_buf)) {
        return false;
    }
    b = vol->sector_buf;

    bytes_per_sector = dsi_read_le16(b + 11u);
    if (bytes_per_sector != FAT_SECTOR_SIZE) {
        return false;
    }

    if (b[13] == 0u || (b[13] & (u8)(b[13] - 1u)) != 0u) {
        return false;   /* sectors per cluster must be a power of two */
    }

    reserved = dsi_read_le16(b + 14u);
    if (b[16] == 0u) {
        return false;   /* number of FATs must be >= 1 */
    }

    root_entries = dsi_read_le16(b + 17u);
    total16 = dsi_read_le16(b + 19u);
    fat_size16 = dsi_read_le16(b + 22u);
    total32 = dsi_read_le32(b + 32u);
    fat_size32 = dsi_read_le32(b + 36u);
    root_cluster = dsi_read_le32(b + 44u);

    total = (total16 != 0u) ? (u32)total16 : total32;
    fat_size = (fat_size16 != 0u) ? (u32)fat_size16 : fat_size32;

    if (total == 0u || fat_size == 0u || reserved == 0u) {
        return false;
    }
    if (root_entries != 0u && fat_size16 == 0u) {
        return false;   /* inconsistent FAT16 BPB */
    }

    /* Reject a volume whose type string explicitly says FAT12. */
    if (dsi_memcmp(b + 54u, "FAT12   ", 8u) == 0) {
        return false;
    }

    if (root_entries == 0u) {
        if (root_cluster < 2u) {
            return false;
        }
        vol->type = FAT_TYPE_FAT32;
    } else {
        vol->type = FAT_TYPE_FAT16;
    }

    root_sectors = ((u32)root_entries * FAT_DIR_ENTRY_SIZE +
                    (FAT_SECTOR_SIZE - 1u)) / FAT_SECTOR_SIZE;

    overhead = (u32)reserved + (u32)b[16] * fat_size + root_sectors;
    if (total <= overhead) {
        return false;
    }
    cluster_count = (total - overhead) / (u32)b[13];
    if (cluster_count == 0u) {
        return false;
    }

    if (vol->type == FAT_TYPE_FAT32 && root_cluster > cluster_count + 1u) {
        return false;
    }

    vol->bytes_per_sector = bytes_per_sector;
    vol->sectors_per_cluster = b[13];
    vol->reserved_sectors = reserved;
    vol->num_fats = b[16];
    vol->root_entry_count = root_entries;
    vol->total_sectors = total;
    vol->fat_size = fat_size;
    vol->root_cluster = root_cluster;

    vol->partition_lba = base;
    vol->fat_start_lba = base + reserved;
    vol->root_start_lba = vol->fat_start_lba + (u32)b[16] * fat_size;
    vol->root_sectors = root_sectors;
    vol->data_start_lba = vol->root_start_lba + root_sectors;
    vol->cluster_count = cluster_count;
    vol->max_cluster = cluster_count + 1u;
    return true;
}

bool fat_mount(fat_volume_t *vol, fat_read_sector_fn read_fn, void *ctx)
{
    u32 part_lba = 0u;
    bool have_partition = false;
    u32 i;

    if (vol == NULL || read_fn == NULL) {
        return false;
    }

    dsi_memset(vol, 0, sizeof(*vol));
    vol->read = read_fn;
    vol->ctx = ctx;

    /* 1. Look for an MBR with a FAT partition entry. */
    if (!vol_read_sector(vol, 0u, vol->sector_buf)) {
        return false;
    }
    if (dsi_read_le16(vol->sector_buf + MBR_SIG_OFF) == MBR_SIG_VALUE) {
        for (i = 0u; i < MBR_PART_COUNT; i++) {
            const u8 *pe = vol->sector_buf + MBR_PART_OFF +
                           i * MBR_PART_STRIDE;
            u32 lba = dsi_read_le32(pe + MBR_PART_LBA_OFF);
            u32 cnt = dsi_read_le32(pe + MBR_PART_CNT_OFF);

            if (lba != 0u && cnt != 0u && part_type_is_fat(pe[MBR_PART_TYPE_OFF])) {
                part_lba = lba;
                have_partition = true;
                break;
            }
        }
    }

    /* 2. Prefer the partition; fall back to a superfloppy at LBA 0. */
    vol->io_error = false;
    if (have_partition && fat_parse_bpb(vol, part_lba)) {
        return true;
    }

    vol->io_error = false;
    if (fat_parse_bpb(vol, 0u)) {
        return true;
    }

    vol->type = FAT_TYPE_NONE;
    return false;
}

/* ---- FAT chain --------------------------------------------------------- */

u32 fat_get_next_cluster(fat_volume_t *vol, u32 cluster)
{
    u32 offset;
    u32 sector;
    u32 off;

    if (vol == NULL || cluster < 2u || cluster > vol->max_cluster) {
        return FAT_INVALID_CLUSTER;
    }

    if (vol->type == FAT_TYPE_FAT32) {
        offset = cluster * 4u;
    } else if (vol->type == FAT_TYPE_FAT16) {
        offset = cluster * 2u;
    } else {
        return FAT_INVALID_CLUSTER;
    }

    sector = vol->fat_start_lba + offset / FAT_SECTOR_SIZE;
    off = offset % FAT_SECTOR_SIZE;

    if (!vol_read_sector(vol, sector, vol->sector_buf)) {
        return FAT_INVALID_CLUSTER;
    }

    if (vol->type == FAT_TYPE_FAT32) {
        return dsi_read_le32(vol->sector_buf + off) & FAT32_MASK;
    }
    return dsi_read_le16(vol->sector_buf + off);
}

bool fat_is_eoc(const fat_volume_t *vol, u32 cluster)
{
    if (vol == NULL) {
        return true;
    }
    if (vol->type == FAT_TYPE_FAT32) {
        return cluster >= FAT32_EOC_MIN;
    }
    return cluster >= FAT16_EOC_MIN;
}

/* ---- Directory iteration ----------------------------------------------- */

void fat_dir_iter_init(fat_volume_t *vol, fat_dir_iter_t *it, bool nds_only)
{
    if (it == NULL) {
        return;
    }

    dsi_memset(it, 0, sizeof(*it));
    it->vol = vol;
    it->nds_only = nds_only;

    if (vol == NULL || vol->type == FAT_TYPE_NONE) {
        it->finished = true;
        return;
    }
    vol->io_error = false;

    if (vol->type == FAT_TYPE_FAT32) {
        it->is_root_fixed = false;
        it->cur_cluster = vol->root_cluster;
        it->sectors_left_in_cluster = vol->sectors_per_cluster;
        it->cur_lba = cluster_lba(vol, vol->root_cluster);
    } else {
        it->is_root_fixed = true;
        it->root_sectors_left = vol->root_sectors;
        it->cur_lba = vol->root_start_lba;
    }
    it->buffer_valid = false;
    it->finished = false;
}

/* Make the next directory sector current. Returns false at end of directory. */
static bool dir_next_sector(fat_dir_iter_t *it)
{
    fat_volume_t *vol = it->vol;

    if (it->is_root_fixed) {
        if (it->root_sectors_left == 0u) {
            return false;
        }
        it->root_sectors_left--;
        if (!vol_read_sector(vol, it->cur_lba, vol->sector_buf)) {
            return false;
        }
        it->cur_lba++;
    } else {
        if (it->sectors_left_in_cluster == 0u) {
            u32 next = fat_get_next_cluster(vol, it->cur_cluster);

            if (vol->io_error || !cluster_in_use(vol, next) ||
                fat_is_eoc(vol, next) || cluster_is_bad(vol, next)) {
                return false;
            }
            it->cur_cluster = next;
            it->sectors_left_in_cluster = vol->sectors_per_cluster;
            it->cur_lba = cluster_lba(vol, next);
        }
        if (!vol_read_sector(vol, it->cur_lba, vol->sector_buf)) {
            return false;
        }
        it->cur_lba++;
        it->sectors_left_in_cluster--;
    }

    it->buffer_valid = true;
    it->entry_index = 0u;
    return true;
}

bool fat_dir_next(fat_dir_iter_t *it, fat_file_entry_t *out)
{
    if (it == NULL || out == NULL || it->vol == NULL) {
        return false;
    }

    for (;;) {
        const u8 *e;
        u8 attr;

        if (it->finished) {
            return false;
        }

        if (!it->buffer_valid ||
            it->entry_index >= FAT_DIR_ENTRIES_PER_SECTOR) {
            if (!dir_next_sector(it)) {
                it->finished = true;
                return false;
            }
        }

        e = it->vol->sector_buf + it->entry_index * FAT_DIR_ENTRY_SIZE;
        it->entry_index++;

        if (e[0] == 0x00u) {
            it->finished = true;   /* end of directory */
            return false;
        }
        if (e[0] == 0xE5u) {
            continue;              /* deleted */
        }

        attr = e[11];
        if ((attr & 0x0Fu) == 0x0Fu) {
            continue;              /* long file name */
        }
        if ((attr & 0x08u) != 0u) {
            continue;              /* volume label */
        }
        if ((attr & 0x10u) != 0u) {
            continue;              /* subdirectory */
        }
        if (it->nds_only && !short_name_is_nds(e)) {
            continue;
        }

        parse_short_name(e, out->name);
        out->attributes = attr;
        out->start_cluster = (u32)dsi_read_le16(e + 26u);
        if (it->vol->type == FAT_TYPE_FAT32) {
            out->start_cluster |=
                (u32)dsi_read_le16(e + 20u) << 16;
        }
        out->file_size = dsi_read_le32(e + 28u);
        return true;
    }
}

bool fat_is_nds_entry(const fat_file_entry_t *entry)
{
    const char *dot;
    const char *p;

    if (entry == NULL) {
        return false;
    }

    /* Find the extension separator. */
    dot = NULL;
    for (p = entry->name; *p != '\0'; p++) {
        if (*p == '.') {
            dot = p;
        }
    }
    if (dot == NULL || dot[1] == '\0' || dot[2] == '\0' || dot[3] == '\0' ||
        dot[4] != '\0') {
        return false;
    }

    {
        char a = dot[1];
        char b = dot[2];
        char c = dot[3];

        if (a >= 'a' && a <= 'z') { a = (char)(a - ('a' - 'A')); }
        if (b >= 'a' && b <= 'z') { b = (char)(b - ('a' - 'A')); }
        if (c >= 'a' && c <= 'z') { c = (char)(c - ('a' - 'A')); }

        return (a == 'N' && b == 'D' && c == 'S') ||
               (a == 'S' && b == 'R' && c == 'L');
    }
}

/* ---- File data --------------------------------------------------------- */

bool fat_read_file(fat_volume_t *vol, const fat_file_entry_t *entry,
                   u8 *dest_buf, u32 max_len, u32 *out_len)
{
    u32 cluster;
    u32 remaining;
    u32 expected;
    u32 written = 0u;
    u32 cluster_bytes;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (vol == NULL || entry == NULL || dest_buf == NULL) {
        return false;
    }
    vol->io_error = false;

    expected = (entry->file_size > max_len) ? max_len : entry->file_size;
    remaining = expected;
    cluster = entry->start_cluster;
    cluster_bytes = (u32)vol->sectors_per_cluster * FAT_SECTOR_SIZE;

    while (remaining != 0u) {
        u32 base;
        u32 chunk;
        u32 s;

        if (!cluster_in_use(vol, cluster) || cluster_is_bad(vol, cluster)) {
            break;   /* corrupt / truncated chain */
        }

        base = cluster_lba(vol, cluster);
        chunk = (remaining > cluster_bytes) ? cluster_bytes : remaining;

        for (s = 0u; s < chunk; s += FAT_SECTOR_SIZE) {
            u32 n = chunk - s;

            if (n > FAT_SECTOR_SIZE) {
                n = FAT_SECTOR_SIZE;
            }
            if (!vol_read_sector(vol, base + s / FAT_SECTOR_SIZE,
                                 vol->sector_buf)) {
                if (out_len != NULL) {
                    *out_len = written;
                }
                return false;
            }
            dsi_memcpy(dest_buf + written, vol->sector_buf, n);
            written += n;
        }

        remaining -= chunk;
        if (remaining == 0u) {
            break;
        }

        cluster = fat_get_next_cluster(vol, cluster);
        if (vol->io_error || fat_is_eoc(vol, cluster)) {
            break;
        }
    }

    if (out_len != NULL) {
        *out_len = written;
    }
    return written == expected;
}
