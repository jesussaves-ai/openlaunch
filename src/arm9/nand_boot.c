/*
 * src/arm9/nand_boot.c
 *
 * ARM9-side DSi NAND glue. See include/arm9/nand_boot.h.
 *
 * The eMMC NAND controller is ARM7-only, so every read here is delegated over
 * IPC with IPC_STORAGE_DEV_EMMC, mirroring src/arm9/storage.c's SD path. The
 * module is strictly read-only (AGENTS.md §5): no write command is ever issued.
 */
#include "arm9/nand_boot.h"
#include "arm9/storage.h"
#include "common/mem.h"

/* Device selector for the internal eMMC / NAND. */
#define NAND_DEVICE  IPC_STORAGE_DEV_EMMC

bool arm9_nand_read_sectors(u32 lba, void *dst, u32 count)
{
    return arm9_storage_read_sectors(NAND_DEVICE, lba, dst, count);
}

/* ---- FAT backend over the delegated eMMC reader ----------------------- */

typedef struct {
    u32 base;   /* partition start sector added to every read */
} nand_fat_ctx_t;

static nand_fat_ctx_t s_fat_ctx;

static bool nand_fat_read(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    nand_fat_ctx_t *c = (nand_fat_ctx_t *)ctx;

    return arm9_nand_read_sectors(c->base + lba, out, 1u);
}

bool arm9_nand_mount_fat(fat_volume_t *vol, u32 partition_lba)
{
    if (vol == NULL) {
        return false;
    }
    s_fat_ctx.base = partition_lba;
    return fat_mount(vol, nand_fat_read, &s_fat_ctx);
}

/* ---- LBA-0 partition-table probe -------------------------------------- */

/*
 * Probe buffer. Lives in .bss (shared Main RAM) so the ARM7 can fill it
 * directly over IPC; never DTCM.
 */
static u8 s_nand_sector0[IPC_STORAGE_SECTOR_SIZE];

void arm9_nand_probe(arm9_nand_probe_t *out)
{
    nand_table_t tbl;
    s32 fat;

    if (out == NULL) {
        return;
    }

    out->ok = false;
    out->signature_ok = false;
    out->signature = 0u;
    out->format = NAND_TABLE_NONE;
    out->partitions = 0u;
    out->fat_present = false;
    out->fat_lba = 0u;
    out->fat_sectors = 0u;
    out->total_sectors = 0u;

    if (!arm9_nand_read_sectors(0u, s_nand_sector0, 1u)) {
        return;
    }

    out->ok = true;
    out->signature = dsi_read_le16(s_nand_sector0 + NAND_MBR_SIG_OFF);
    out->signature_ok = (out->signature == NAND_MBR_SIGNATURE);

    if (!nand_parse_table(s_nand_sector0, &tbl)) {
        return;
    }

    out->format = (u8)tbl.format;
    out->partitions = tbl.count;
    out->total_sectors = tbl.total_sectors;

    fat = nand_find_fat_partition(&tbl);
    if (fat >= 0) {
        out->fat_present = true;
        out->fat_lba = tbl.partitions[(u32)fat].lba;
        out->fat_sectors = tbl.partitions[(u32)fat].sector_count;
    } else if (tbl.format == NAND_TABLE_TWL &&
               tbl.partitions[0].present) {
        /*
         * The TWL form carries no FAT type byte. Report the first partition
         * as the candidate; the caller confirms it by attempting a mount.
         */
        out->fat_lba = tbl.partitions[0].lba;
        out->fat_sectors = tbl.partitions[0].sector_count;
    }
}

/* ---- Launcher resolution ---------------------------------------------- */

static const char *const s_launcher_paths[] = {
    NAND_SYSTEM_MENU_PATH,
    NAND_HIYACFW_LAUNCHER_PATH,
};

/* Bounded NUL-terminating copy. */
static void nand_copy_str(char *dst, u32 cap, const char *src)
{
    u32 i = 0u;

    if (dst == NULL || cap == 0u) {
        return;
    }
    if (src != NULL) {
        while (src[i] != '\0' && i + 1u < cap) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

bool arm9_nand_find_launcher(fat_volume_t *vol, char *path_out, u32 cap)
{
    u32 i;

    if (vol == NULL || path_out == NULL || cap < 2u) {
        return false;
    }
    path_out[0] = '\0';

    for (i = 0u; i < ARRAY_SIZE(s_launcher_paths); i++) {
        fat_file_entry_t e;

        if (nand_resolve_path(vol, s_launcher_paths[i], &e) && !e.is_dir) {
            nand_copy_str(path_out, cap, s_launcher_paths[i]);
            return true;
        }
    }
    return false;
}
