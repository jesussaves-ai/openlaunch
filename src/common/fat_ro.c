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

bool fat_mbr_find_fat_partition(const u8 sector[FAT_SECTOR_SIZE],
                                fat_mbr_partition_t *out)
{
    u32 i;

    if (sector == NULL) {
        return false;
    }
    if (dsi_read_le16(sector + MBR_SIG_OFF) != MBR_SIG_VALUE) {
        return false;
    }

    for (i = 0u; i < MBR_PART_COUNT; i++) {
        const u8 *pe = sector + MBR_PART_OFF + i * MBR_PART_STRIDE;
        u32 lba = dsi_read_le32(pe + MBR_PART_LBA_OFF);
        u32 cnt = dsi_read_le32(pe + MBR_PART_CNT_OFF);
        u8 type = pe[MBR_PART_TYPE_OFF];

        if (lba != 0u && cnt != 0u && part_type_is_fat(type)) {
            if (out != NULL) {
                out->present = true;
                out->type = type;
                out->lba = lba;
                out->sector_count = cnt;
            }
            return true;
        }
    }

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
    }
    return false;
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
    {
        fat_mbr_partition_t part;

        if (fat_mbr_find_fat_partition(vol->sector_buf, &part)) {
            part_lba = part.lba;
            have_partition = true;
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
        out->is_dir = false;   /* fat_dir_next() skips subdirectories */
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

/* ---- Bounded directory-cluster listing / path resolution -------------- */

/* Visitor over one directory entry; returning false stops the walk. */
typedef bool (*fat_dir_visit_fn)(void *ctx, const fat_file_entry_t *entry);

/*
 * Walk the directory whose cluster chain starts at `start_cluster`
 * (0 = root), invoking `visit` for every regular file and subdirectory entry.
 * Long-file-name, volume-label and deleted entries are skipped and the 0x00
 * end-of-directory marker stops the walk. The chain is capped at
 * `cluster_count + 2` hops and self-loops / two-cluster ping-pong chains are
 * rejected, mirroring fat_read_file_range().
 */
static fat_dir_status_t fat_dir_walk(fat_volume_t *vol, u32 start_cluster,
                                     fat_dir_visit_fn visit, void *ctx)
{
    bool root_fixed;
    u32 cluster = 0u;
    u32 lba;
    u32 sectors_left;
    u32 prev_cluster = 0u;
    u32 hops = 0u;
    u32 max_hops;

    if (vol == NULL || vol->type == FAT_TYPE_NONE || visit == NULL) {
        return FAT_DIR_ERR_ARG;
    }

    if (vol->type == FAT_TYPE_FAT16 && start_cluster == 0u) {
        root_fixed = true;
        sectors_left = vol->root_sectors;
        lba = vol->root_start_lba;
        max_hops = 0u;   /* unused: the FAT16 fixed root has no chain */
    } else {
        root_fixed = false;
        cluster = (start_cluster == 0u) ? vol->root_cluster : start_cluster;
        if (!cluster_in_use(vol, cluster) || cluster_is_bad(vol, cluster)) {
            return FAT_DIR_ERR_BAD_CLUSTER;
        }
        sectors_left = vol->sectors_per_cluster;
        lba = cluster_lba(vol, cluster);
        max_hops = vol->cluster_count + 2u;
    }

    if (sectors_left == 0u) {
        return FAT_DIR_OK;   /* empty directory */
    }

    for (;;) {
        u32 idx;

        /* Advance to the next cluster when the current one is exhausted. */
        if (sectors_left == 0u) {
            u32 next;

            if (root_fixed) {
                return FAT_DIR_OK;
            }
            next = fat_get_next_cluster(vol, cluster);
            if (vol->io_error) {
                return FAT_DIR_ERR_IO;
            }
            if (fat_is_eoc(vol, next)) {
                return FAT_DIR_OK;
            }
            if (next == 0u || cluster_is_bad(vol, next) ||
                !cluster_in_use(vol, next)) {
                return FAT_DIR_ERR_BAD_CLUSTER;
            }
            if (next == cluster || next == prev_cluster) {
                return FAT_DIR_ERR_LOOP;
            }
            if (++hops > max_hops) {
                return FAT_DIR_ERR_LOOP;
            }
            prev_cluster = cluster;
            cluster = next;
            sectors_left = vol->sectors_per_cluster;
            lba = cluster_lba(vol, cluster);
        }

        if (!vol_read_sector(vol, lba, vol->sector_buf)) {
            return FAT_DIR_ERR_IO;
        }
        lba++;
        sectors_left--;

        for (idx = 0u; idx < FAT_DIR_ENTRIES_PER_SECTOR; idx++) {
            const u8 *e = vol->sector_buf + idx * FAT_DIR_ENTRY_SIZE;
            fat_file_entry_t entry;
            u8 attr;

            if (e[0] == 0x00u) {
                return FAT_DIR_OK;   /* end of directory */
            }
            if (e[0] == 0xE5u) {
                continue;            /* deleted */
            }

            attr = e[11];
            if ((attr & FAT_ATTR_LFN) == FAT_ATTR_LFN) {
                continue;            /* long file name */
            }
            if ((attr & FAT_ATTR_VOLUME_ID) != 0u) {
                continue;            /* volume label */
            }

            dsi_memset(&entry, 0, sizeof(entry));
            parse_short_name(e, entry.name);
            entry.attributes = attr;
            entry.is_dir = (attr & FAT_ATTR_DIRECTORY) != 0u;
            entry.start_cluster = (u32)dsi_read_le16(e + 26u);
            if (vol->type == FAT_TYPE_FAT32) {
                entry.start_cluster |=
                    (u32)dsi_read_le16(e + 20u) << 16;
            }
            entry.file_size = dsi_read_le32(e + 28u);

            if (!visit(ctx, &entry)) {
                return FAT_DIR_OK;
            }
        }
    }
}

typedef struct {
    fat_file_entry_t *entries;
    u32 max;
    u32 count;
} fat_dir_collect_t;

static bool fat_dir_collect_visit(void *ctx, const fat_file_entry_t *entry)
{
    fat_dir_collect_t *c = (fat_dir_collect_t *)ctx;

    if (c->count >= c->max) {
        return false;
    }
    c->entries[c->count] = *entry;
    c->count++;
    return true;
}

fat_dir_status_t fat_list_dir_cluster(fat_volume_t *vol, u32 start_cluster,
                                      fat_file_entry_t *entries,
                                      u32 max_entries, u32 *out_count)
{
    fat_dir_collect_t c;
    fat_dir_status_t st;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if (vol == NULL || entries == NULL || max_entries == 0u ||
        vol->type == FAT_TYPE_NONE) {
        return FAT_DIR_ERR_ARG;
    }

    vol->io_error = false;
    c.entries = entries;
    c.max = max_entries;
    c.count = 0u;

    st = fat_dir_walk(vol, start_cluster, fat_dir_collect_visit, &c);

    if (out_count != NULL) {
        *out_count = c.count;
    }
    return st;
}

/* ASCII case-insensitive equality of two NUL-terminated names. */
static bool fat_name_equals_ci(const char *a, const char *b)
{
    u32 i = 0u;

    for (;;) {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'a' && ca <= 'z') { ca = (char)(ca - ('a' - 'A')); }
        if (cb >= 'a' && cb <= 'z') { cb = (char)(cb - ('a' - 'A')); }
        if (ca != cb) {
            return false;
        }
        if (ca == '\0') {
            return true;
        }
        i++;
    }
}

typedef struct {
    const char *name;
    bool require_dir;
    fat_file_entry_t *out;
    bool found;
} fat_dir_find_t;

static bool fat_dir_find_visit(void *ctx, const fat_file_entry_t *entry)
{
    fat_dir_find_t *f = (fat_dir_find_t *)ctx;

    if (fat_name_equals_ci(entry->name, f->name) &&
        (!f->require_dir || entry->is_dir)) {
        *f->out = *entry;
        f->found = true;
        return false;
    }
    return true;
}

static bool fat_dir_find(fat_volume_t *vol, u32 start_cluster,
                         const char *name, bool require_dir,
                         fat_file_entry_t *out)
{
    fat_dir_find_t f;

    f.name = name;
    f.require_dir = require_dir;
    f.out = out;
    f.found = false;
    (void)fat_dir_walk(vol, start_cluster, fat_dir_find_visit, &f);
    return f.found;
}

bool fat_find_path(fat_volume_t *vol, const char *path_str,
                   fat_file_entry_t *out_entry)
{
    const char *p;
    const char *slash = NULL;
    const char *q;
    char dir[FAT_NAME_MAX];
    u32 dir_len;
    fat_file_entry_t dir_entry;

    if (vol == NULL || path_str == NULL || out_entry == NULL ||
        vol->type == FAT_TYPE_NONE) {
        return false;
    }
    vol->io_error = false;

    /* "/BOOT.NDS" and "BOOT.NDS" are equivalent. */
    p = path_str;
    while (*p == '/') {
        p++;
    }
    if (*p == '\0') {
        return false;
    }

    for (q = p; *q != '\0'; q++) {
        if (*q == '/') {
            slash = q;
            break;
        }
    }

    if (slash == NULL) {
        return fat_dir_find(vol, 0u, p, false, out_entry);
    }

    /* Only a single directory level is supported. */
    for (q = slash + 1u; *q != '\0'; q++) {
        if (*q == '/') {
            return false;
        }
    }
    if (slash[1] == '\0') {
        return false;   /* trailing slash: no file name */
    }

    dir_len = (u32)(slash - p);
    if (dir_len == 0u || dir_len >= FAT_NAME_MAX) {
        return false;
    }
    dsi_memcpy(dir, p, dir_len);
    dir[dir_len] = '\0';

    if (!fat_dir_find(vol, 0u, dir, true, &dir_entry)) {
        return false;
    }
    if (!dir_entry.is_dir) {
        return false;   /* a file used as a directory */
    }
    if (dir_entry.start_cluster < 2u) {
        return false;   /* FAT16 empty directory (start cluster 0) */
    }

    return fat_dir_find(vol, dir_entry.start_cluster, slash + 1u, false,
                        out_entry);
}

/* ---- File data --------------------------------------------------------- */

/*
 * Read `len` bytes at byte `offset` within `entry`, following the FAT16/FAT32
 * cluster chain. See include/common/fat_ro.h for the guard contract.
 *
 * The function never trusts the on-disk chain: it bounds every hop, rejects
 * free/bad/out-of-range clusters, detects trivial cycles (self loop and
 * two-cluster ping-pong) and treats an early EOC as truncation.
 */
bool fat_read_file_range(fat_volume_t *vol, const fat_file_entry_t *entry,
                         u32 offset, u8 *dest_buf, u32 len,
                         u32 *out_len, fat_chain_status_t *out_status)
{
    u32 cluster_bytes;
    u32 cluster;
    u32 prev_cluster;
    u32 skip;
    u32 pos;
    u32 remaining;
    u32 written = 0u;
    u32 max_hops;
    u32 hops = 0u;
    fat_chain_status_t status = FAT_CHAIN_OK;

    if (out_len != NULL) {
        *out_len = 0u;
    }
    if (out_status != NULL) {
        *out_status = FAT_CHAIN_ERR_ARG;
    }

    if (vol == NULL || entry == NULL || dest_buf == NULL ||
        vol->type == FAT_TYPE_NONE) {
        return false;
    }
    vol->io_error = false;

    if (!cluster_in_use(vol, entry->start_cluster)) {
        if (out_status != NULL) {
            *out_status = FAT_CHAIN_ERR_BAD_CLUSTER;
        }
        return false;
    }

    /* A zero-byte request at EOF is a valid no-op. */
    if (offset >= entry->file_size) {
        status = (offset == entry->file_size) ? FAT_CHAIN_OK
                                              : FAT_CHAIN_ERR_TRUNCATED;
        if (out_status != NULL) {
            *out_status = status;
        }
        return status == FAT_CHAIN_OK;
    }
    if (len == 0u) {
        if (out_status != NULL) {
            *out_status = FAT_CHAIN_OK;
        }
        return true;
    }

    cluster_bytes = (u32)vol->sectors_per_cluster * FAT_SECTOR_SIZE;
    if (cluster_bytes == 0u) {
        if (out_status != NULL) {
            *out_status = FAT_CHAIN_ERR_BAD_CLUSTER;
        }
        return false;
    }

    /* Never walk more clusters than the file can possibly use, plus slack. */
    max_hops = entry->file_size / cluster_bytes + 2u;

    /* Clamp the request to what the file actually contains. */
    if (len > entry->file_size - offset) {
        len = entry->file_size - offset;
    }

    cluster = entry->start_cluster;
    prev_cluster = 0u;
    remaining = len;
    pos = offset % cluster_bytes;
    skip = offset / cluster_bytes;

    /* 1. Skip whole clusters up to the requested offset. */
    while (skip > 0u) {
        u32 next;

        if (cluster_is_bad(vol, cluster)) {
            status = FAT_CHAIN_ERR_BAD_CLUSTER;
            goto done;
        }
        next = fat_get_next_cluster(vol, cluster);
        if (vol->io_error) {
            status = FAT_CHAIN_ERR_IO;
            goto done;
        }
        if (fat_is_eoc(vol, next)) {
            status = FAT_CHAIN_ERR_TRUNCATED;
            goto done;
        }
        if (next == 0u || cluster_is_bad(vol, next) ||
            !cluster_in_use(vol, next)) {
            status = FAT_CHAIN_ERR_BAD_CLUSTER;
            goto done;
        }
        if (++hops > max_hops) {
            status = FAT_CHAIN_ERR_LOOP;
            goto done;
        }
        prev_cluster = cluster;
        cluster = next;
        skip--;
    }

    /* 2. Stream the requested bytes. */
    while (remaining > 0u) {
        u32 base;
        u32 avail;
        u32 chunk;
        u32 s;

        if (cluster_is_bad(vol, cluster)) {
            status = FAT_CHAIN_ERR_BAD_CLUSTER;
            goto done;
        }

        avail = cluster_bytes - pos;
        chunk = (remaining < avail) ? remaining : avail;
        base = cluster_lba(vol, cluster);

        for (s = 0u; s < chunk; ) {
            u32 cluster_off = pos + s;
            u32 in_sector = cluster_off % FAT_SECTOR_SIZE;
            u32 n = FAT_SECTOR_SIZE - in_sector;

            if (n > chunk - s) {
                n = chunk - s;
            }
            if (!vol_read_sector(vol, base + cluster_off / FAT_SECTOR_SIZE,
                                 vol->sector_buf)) {
                status = FAT_CHAIN_ERR_IO;
                goto done;
            }
            dsi_memcpy(dest_buf + written, vol->sector_buf + in_sector, n);
            written += n;
            s += n;
        }

        remaining -= chunk;
        pos = 0u;
        if (remaining == 0u) {
            break;
        }

        {
            u32 next = fat_get_next_cluster(vol, cluster);

            if (vol->io_error) {
                status = FAT_CHAIN_ERR_IO;
                goto done;
            }
            if (fat_is_eoc(vol, next)) {
                status = FAT_CHAIN_ERR_TRUNCATED;
                goto done;
            }
            if (next == 0u || cluster_is_bad(vol, next) ||
                !cluster_in_use(vol, next)) {
                status = FAT_CHAIN_ERR_BAD_CLUSTER;
                goto done;
            }
            /* Reject a corrupt self loop or a two-cluster ping-pong. */
            if (next == cluster || next == prev_cluster) {
                status = FAT_CHAIN_ERR_LOOP;
                goto done;
            }
            if (++hops > max_hops) {
                status = FAT_CHAIN_ERR_LOOP;
                goto done;
            }
            prev_cluster = cluster;
            cluster = next;
        }
    }

done:
    if (out_len != NULL) {
        *out_len = written;
    }
    if (out_status != NULL) {
        *out_status = status;
    }
    return status == FAT_CHAIN_OK;
}

bool fat_read_file(fat_volume_t *vol, const fat_file_entry_t *entry,
                   u8 *dest_buf, u32 max_len, u32 *out_len)
{
    return fat_read_file_range(vol, entry, 0u, dest_buf, max_len, out_len,
                               NULL);
}

const char *fat_chain_status_text(fat_chain_status_t status)
{
    switch (status) {
    case FAT_CHAIN_OK:          return "OK";
    case FAT_CHAIN_ERR_ARG:     return "BAD ARGUMENT";
    case FAT_CHAIN_ERR_IO:      return "READ ERROR";
    case FAT_CHAIN_ERR_BAD_CLUSTER: return "BAD CLUSTER";
    case FAT_CHAIN_ERR_LOOP:    return "FAT CHAIN LOOP";
    case FAT_CHAIN_ERR_TRUNCATED: return "CHAIN TRUNCATED";
    default:                    return "UNKNOWN";
    }
}
