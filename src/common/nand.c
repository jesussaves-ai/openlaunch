/*
 * src/common/nand.c
 *
 * Read-only DSi NAND partition-table parser and boot-redirect policy.
 * See include/common/nand.h.
 *
 * This file is pure: it decodes a caller-supplied 512-byte sector and never
 * touches a device. The ARM9 glue that actually reads the eMMC NAND over IPC
 * lives in src/arm9/nand_boot.c.
 */
#include "common/nand.h"
#include "common/mem.h"

/* ---- Partition-type classification ------------------------------------ */

bool nand_partition_is_fat(u8 type)
{
    return type == FAT_PART_TYPE_FAT16_SMALL ||
           type == FAT_PART_TYPE_FAT16 ||
           type == FAT_PART_TYPE_FAT32 ||
           type == FAT_PART_TYPE_FAT32_LBA ||
           type == FAT_PART_TYPE_FAT16_LBA;
}

/* ---- PC-style MBR ----------------------------------------------------- */

bool nand_parse_mbr_pc(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out)
{
    u32 i;

    if (out == NULL) {
        return false;
    }
    dsi_memset(out, 0, sizeof(*out));
    out->format = NAND_TABLE_NONE;
    out->largest_fat = -1;

    if (sector == NULL ||
        dsi_read_le16(sector + NAND_MBR_SIG_OFF) != NAND_MBR_SIGNATURE) {
        return false;
    }

    out->format = NAND_TABLE_PC_MBR;

    for (i = 0u; i < NAND_MBR_ENTRY_COUNT; i++) {
        const u8 *e = sector + NAND_MBR_TABLE_OFF + i * NAND_MBR_ENTRY_SIZE;
        nand_partition_t *p = &out->partitions[i];
        u8 type = e[NAND_MBR_TYPE_OFF];
        u32 lba = dsi_read_le32(e + NAND_MBR_LBA_OFF);
        u32 count = dsi_read_le32(e + NAND_MBR_COUNT_OFF);

        p->type = type;
        p->lba = lba;
        p->sector_count = count;
        p->present = (type != 0u || lba != 0u || count != 0u);
        p->fat = nand_partition_is_fat(type) && lba != 0u && count != 0u;

        if (p->present) {
            out->count++;
            if (lba + count > out->total_sectors) {
                out->total_sectors = lba + count;
            }
        }
        if (p->fat) {
            out->fat_count++;
            if (out->largest_fat < 0 ||
                count > out->partitions[(u32)out->largest_fat].sector_count) {
                out->largest_fat = (s32)i;
            }
        }
    }

    return true;
}

/* ---- TWL / HiyaCFW {offset, size} table ------------------------------- */

bool nand_parse_mbr_twl(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out)
{
    u32 i;

    if (out == NULL) {
        return false;
    }
    dsi_memset(out, 0, sizeof(*out));
    out->format = NAND_TABLE_NONE;
    out->largest_fat = -1;

    if (sector == NULL ||
        dsi_read_le16(sector + NAND_MBR_SIG_OFF) != NAND_MBR_SIGNATURE) {
        return false;
    }

    out->format = NAND_TABLE_TWL;

    for (i = 0u; i < NAND_MBR_ENTRY_COUNT; i++) {
        const u8 *e = sector + NAND_MBR_TABLE_OFF + i * NAND_MBR_ENTRY_SIZE;
        nand_partition_t *p = &out->partitions[i];
        u32 off = dsi_read_le32(e + NAND_TWL_OFFSET_OFF);
        u32 size = dsi_read_le32(e + NAND_TWL_SIZE_OFF);

        /*
         * The TWL form carries no FAT type byte; the caller probes the
         * partition's BPB to classify it. Mark it non-FAT here.
         */
        p->type = 0u;
        p->lba = off;
        p->sector_count = size;
        p->present = (off != 0u || size != 0u);
        p->fat = false;

        if (p->present) {
            out->count++;
            if (off + size > out->total_sectors) {
                out->total_sectors = off + size;
            }
        }
    }

    return true;
}

bool nand_parse_table(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out)
{
    nand_table_t pc;
    nand_table_t twl;

    if (out == NULL) {
        return false;
    }
    dsi_memset(out, 0, sizeof(*out));
    out->format = NAND_TABLE_NONE;
    out->largest_fat = -1;

    if (sector == NULL ||
        dsi_read_le16(sector + NAND_MBR_SIG_OFF) != NAND_MBR_SIGNATURE) {
        return false;
    }

    (void)nand_parse_mbr_pc(sector, &pc);
    (void)nand_parse_mbr_twl(sector, &twl);

    /* A PC MBR with a recognisable FAT partition is unambiguous. */
    if (pc.fat_count > 0u) {
        dsi_memcpy(out, &pc, sizeof(*out));
        return true;
    }
    /* Otherwise prefer the TWL {offset,size} interpretation when non-empty. */
    if (twl.count > 0u) {
        dsi_memcpy(out, &twl, sizeof(*out));
        return true;
    }
    if (pc.count > 0u) {
        dsi_memcpy(out, &pc, sizeof(*out));
        return true;
    }
    return false;
}

s32 nand_find_fat_partition(const nand_table_t *tbl)
{
    u32 i;

    if (tbl == NULL) {
        return -1;
    }
    for (i = 0u; i < NAND_MBR_ENTRY_COUNT; i++) {
        if (tbl->partitions[i].fat) {
            return (s32)i;
        }
    }
    return -1;
}

s32 nand_find_largest_fat(const nand_table_t *tbl)
{
    u32 i;
    s32 best = -1;

    if (tbl == NULL) {
        return -1;
    }
    for (i = 0u; i < NAND_MBR_ENTRY_COUNT; i++) {
        if (!tbl->partitions[i].fat) {
            continue;
        }
        if (best < 0 ||
            tbl->partitions[i].sector_count >
                tbl->partitions[(u32)best].sector_count) {
            best = (s32)i;
        }
    }
    return best;
}

/* ---- Deep FAT path resolution ----------------------------------------- */

static char nand_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - ('a' - 'A')) : c;
}

/* Case-insensitive comparison of an 8.3 entry name and a path component. */
static bool nand_name_eq(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (nand_upper(*a) != nand_upper(*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

bool nand_resolve_path(fat_volume_t *vol, const char *path,
                       fat_file_entry_t *out)
{
    fat_file_entry_t page[NAND_DIR_PAGE];
    u32 cur = 0u;          /* 0 selects the root directory */
    u32 depth = 0u;
    const char *p = path;

    if (vol == NULL || path == NULL || out == NULL) {
        return false;
    }

    for (;;) {
        char comp[NAND_COMPONENT_MAX];
        u32 n = 0u;
        u32 got = 0u;
        u32 i;
        s32 found = -1;
        bool is_last;

        while (*p == '/') {
            p++;
        }
        if (*p == '\0') {
            return false;
        }

        while (*p != '\0' && *p != '/' && n + 1u < NAND_COMPONENT_MAX) {
            comp[n++] = *p++;
        }
        /* Consume an over-long component so the path cannot desynchronise. */
        while (*p != '\0' && *p != '/') {
            p++;
        }
        comp[n] = '\0';

        while (*p == '/') {
            p++;
        }
        is_last = (*p == '\0');

        if (depth++ >= NAND_PATH_MAX_DEPTH) {
            return false;
        }

        if (fat_list_dir_cluster(vol, cur, page, NAND_DIR_PAGE, &got) !=
            FAT_DIR_OK) {
            return false;
        }
        for (i = 0u; i < got; i++) {
            if (nand_name_eq(page[i].name, comp)) {
                found = (s32)i;
                break;
            }
        }
        if (found < 0) {
            return false;
        }

        if (is_last) {
            *out = page[(u32)found];
            return true;
        }
        if (!page[(u32)found].is_dir) {
            return false;
        }
        cur = page[(u32)found].start_cluster;
        if (cur < 2u) {
            return false;   /* a directory with no data cluster has no children */
        }
    }
}

/* ---- Boot-redirect policy --------------------------------------------- */

void boot_resolve_nand_target(const nand_boot_context_t *ctx,
                              nand_boot_target_t *out)
{
    if (out == NULL) {
        return;
    }

    out->attempt = false;
    out->path = NAND_SYSTEM_MENU_PATH;
    out->reason = "no redirect";

    if (ctx == NULL) {
        out->reason = "no context";
        return;
    }

    if (ctx->menu_forced) {
        out->reason = "menu forced (B/SELECT)";
        return;
    }
    if (ctx->sd_target_found) {
        out->reason = "SD boot target present";
        return;
    }
    if (ctx->nand_present) {
        out->attempt = true;
        out->reason = "SD target absent: redirect to DSi System Menu";
        return;
    }
    if (ctx->sd_card_present) {
        out->reason = "SD card present, no boot target";
        return;
    }
    out->reason = "no SD card and no NAND volume";
}
