/*
 * tests/test_nand.c
 *
 * Host-side unit tests for the DSi NAND partition-table parser and boot
 * redirect policy (src/common/nand.c), plus a deep FAT path-resolution check
 * against the generated build/sdcard.img when a path is supplied.
 *
 * The partition tables are synthesized as 512-byte sector buffers, so no
 * device or image file is needed for the core checks.
 *
 * Build/run with:  make host-test   (passes build/sdcard.img as argv[1])
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/nand.h"

/* ------------------------------------------------------------------ */
/* Little-endian helpers                                               */
/* ------------------------------------------------------------------ */

static void put32(u8 *p, u32 off, u32 v)
{
    p[off] = (u8)(v & 0xFFu);
    p[off + 1u] = (u8)((v >> 8) & 0xFFu);
    p[off + 2u] = (u8)((v >> 16) & 0xFFu);
    p[off + 3u] = (u8)((v >> 24) & 0xFFu);
}

static void set_sig(u8 *sector)
{
    sector[NAND_MBR_SIG_OFF] = 0x55u;
    sector[NAND_MBR_SIG_OFF + 1u] = 0xAAu;
}

static void set_pc_entry(u8 *sector, u32 i, u8 type, u32 lba, u32 count)
{
    u8 *e = sector + NAND_MBR_TABLE_OFF + i * NAND_MBR_ENTRY_SIZE;

    memset(e, 0, NAND_MBR_ENTRY_SIZE);
    e[NAND_MBR_TYPE_OFF] = type;
    put32(e, NAND_MBR_LBA_OFF, lba);
    put32(e, NAND_MBR_COUNT_OFF, count);
}

static void set_twl_entry(u8 *sector, u32 i, u32 off, u32 size)
{
    u8 *e = sector + NAND_MBR_TABLE_OFF + i * NAND_MBR_ENTRY_SIZE;

    memset(e, 0, NAND_MBR_ENTRY_SIZE);
    put32(e, NAND_TWL_OFFSET_OFF, off);
    put32(e, NAND_TWL_SIZE_OFF, size);
}

/* ------------------------------------------------------------------ */
/* Partition-type classification                                       */
/* ------------------------------------------------------------------ */

static void test_is_fat(void)
{
    assert(nand_partition_is_fat(FAT_PART_TYPE_FAT16_SMALL));
    assert(nand_partition_is_fat(FAT_PART_TYPE_FAT16));
    assert(nand_partition_is_fat(FAT_PART_TYPE_FAT32));
    assert(nand_partition_is_fat(FAT_PART_TYPE_FAT32_LBA));
    assert(nand_partition_is_fat(FAT_PART_TYPE_FAT16_LBA));
    assert(!nand_partition_is_fat(0x00u));
    assert(!nand_partition_is_fat(0x83u));   /* Linux */
    assert(!nand_partition_is_fat(0xFFu));
}

/* ------------------------------------------------------------------ */
/* PC-style MBR                                                        */
/* ------------------------------------------------------------------ */

static void test_pc_mbr(void)
{
    u8 sector[NAND_SECTOR_SIZE];
    nand_table_t tbl;

    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x2000u);
    set_pc_entry(sector, 1u, FAT_PART_TYPE_FAT32, 0x2100u, 0x4000u);
    set_pc_entry(sector, 2u, 0x83u, 0x6100u, 0x0800u);   /* non-FAT */

    assert(nand_parse_mbr_pc(sector, &tbl));
    assert(tbl.format == NAND_TABLE_PC_MBR);
    assert(tbl.count == 3u);
    assert(tbl.fat_count == 2u);
    assert(tbl.partitions[0].present && tbl.partitions[0].fat);
    assert(tbl.partitions[0].type == FAT_PART_TYPE_FAT16);
    assert(tbl.partitions[0].lba == 0x100u);
    assert(tbl.partitions[0].sector_count == 0x2000u);
    assert(tbl.partitions[2].present && !tbl.partitions[2].fat);
    assert(tbl.total_sectors == 0x6100u + 0x0800u);

    /* The larger FAT16/FAT32 entry is partition 1. */
    assert(tbl.largest_fat == 1);
    assert(nand_find_fat_partition(&tbl) == 0);
    assert(nand_find_largest_fat(&tbl) == 1);

    /* Auto-detect keeps the PC interpretation (a FAT type is present). */
    {
        nand_table_t auto_tbl;

        assert(nand_parse_table(sector, &auto_tbl));
        assert(auto_tbl.format == NAND_TABLE_PC_MBR);
        assert(auto_tbl.fat_count == 2u);
    }
}

static void test_pc_mbr_no_signature(void)
{
    u8 sector[NAND_SECTOR_SIZE];
    nand_table_t tbl;

    memset(sector, 0, sizeof(sector));
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x2000u);

    memset(&tbl, 0xAA, sizeof(tbl));
    assert(!nand_parse_mbr_pc(sector, &tbl));
    assert(tbl.format == NAND_TABLE_NONE);
    assert(!nand_parse_table(sector, &tbl));
    assert(tbl.format == NAND_TABLE_NONE);

    /* NULL safety. */
    assert(!nand_parse_mbr_pc(NULL, &tbl));
    assert(!nand_parse_mbr_pc(sector, NULL));
    assert(!nand_parse_table(NULL, &tbl));
    assert(!nand_parse_table(sector, NULL));
    assert(nand_find_fat_partition(NULL) == -1);
    assert(nand_find_largest_fat(NULL) == -1);
}

/* ------------------------------------------------------------------ */
/* TWL / HiyaCFW {offset, size} table                                  */
/* ------------------------------------------------------------------ */

static void test_twl_table(void)
{
    u8 sector[NAND_SECTOR_SIZE];
    nand_table_t tbl;

    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_twl_entry(sector, 0u, 0x100u, 0x1000u);
    set_twl_entry(sector, 1u, 0x2000u, 0x2000u);

    assert(nand_parse_mbr_twl(sector, &tbl));
    assert(tbl.format == NAND_TABLE_TWL);
    assert(tbl.count == 2u);
    assert(tbl.fat_count == 0u);
    assert(tbl.partitions[0].lba == 0x100u);
    assert(tbl.partitions[0].sector_count == 0x1000u);
    assert(!tbl.partitions[0].fat);
    assert(tbl.total_sectors == 0x4000u);

    /* Auto-detect falls through to the TWL interpretation. */
    {
        nand_table_t auto_tbl;

        assert(nand_parse_table(sector, &auto_tbl));
        assert(auto_tbl.format == NAND_TABLE_TWL);
        assert(auto_tbl.count == 2u);
    }

    /* A signature-only sector with no entries is not a usable table. */
    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    assert(!nand_parse_table(sector, &tbl));
}

/* ------------------------------------------------------------------ */
/* Boot-redirect policy                                                */
/* ------------------------------------------------------------------ */

static void test_policy(void)
{
    nand_boot_context_t ctx;
    nand_boot_target_t tgt;

    /* B/SELECT forces the menu. */
    memset(&ctx, 0, sizeof(ctx));
    ctx.menu_forced = true;
    ctx.nand_present = true;
    boot_resolve_nand_target(&ctx, &tgt);
    assert(!tgt.attempt);
    assert(tgt.reason != NULL);

    /* An SD boot target always wins. */
    memset(&ctx, 0, sizeof(ctx));
    ctx.sd_card_present = true;
    ctx.sd_target_found = true;
    ctx.nand_present = true;
    boot_resolve_nand_target(&ctx, &tgt);
    assert(!tgt.attempt);

    /* No SD target, a NAND volume: redirect to the System Menu. */
    memset(&ctx, 0, sizeof(ctx));
    ctx.nand_present = true;
    boot_resolve_nand_target(&ctx, &tgt);
    assert(tgt.attempt);
    assert(tgt.path != NULL);
    assert(strcmp(tgt.path, NAND_SYSTEM_MENU_PATH) == 0);

    /* No SD target and no NAND: nothing to do. */
    memset(&ctx, 0, sizeof(ctx));
    boot_resolve_nand_target(&ctx, &tgt);
    assert(!tgt.attempt);

    /* NULL safety. */
    boot_resolve_nand_target(NULL, &tgt);
    assert(!tgt.attempt);
    assert(tgt.reason != NULL);
    boot_resolve_nand_target(&ctx, NULL);
}

/* ------------------------------------------------------------------ */
/* Retail-encrypted vs decrypted-sandbox image classification          */
/* ------------------------------------------------------------------ */

static void test_image_classify(void)
{
    u8 mbr[NAND_SECTOR_SIZE];
    u8 footer[NAND_SECTOR_SIZE];

    /* Decrypted sandbox: a plaintext 0x55AA MBR and no footer. */
    memset(mbr, 0, sizeof(mbr));
    set_sig(mbr);
    set_pc_entry(mbr, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x2000u);
    assert(!nand_detect_unlaunch_footer(NULL));
    assert(nand_classify_image(mbr, NULL) == NAND_IMAGE_DECRYPTED_SANDBOX);
    {
        u8 zero_footer[NAND_SECTOR_SIZE];

        memset(zero_footer, 0, sizeof(zero_footer));
        assert(nand_classify_image(mbr, zero_footer) ==
               NAND_IMAGE_DECRYPTED_SANDBOX);
    }

    /* Encrypted retail MBR: no 0x55AA signature. */
    memset(mbr, 0x5A, sizeof(mbr));
    assert(nand_classify_image(mbr, NULL) == NAND_IMAGE_ENCRYPTED_RETAIL);

    /* A no$gba / Unlaunch footer always marks a raw retail dump. */
    memset(footer, 0, sizeof(footer));
    memcpy(footer, NAND_FOOTER_MAGIC, NAND_FOOTER_MAGIC_LEN);
    assert(nand_detect_unlaunch_footer(footer));
    assert(nand_classify_image(mbr, footer) == NAND_IMAGE_ENCRYPTED_RETAIL);
    /* Even beside an otherwise-plaintext MBR the footer is treated as retail. */
    memset(mbr, 0, sizeof(mbr));
    set_sig(mbr);
    assert(nand_classify_image(mbr, footer) == NAND_IMAGE_ENCRYPTED_RETAIL);
    assert(nand_classify_image(NULL, footer) == NAND_IMAGE_ENCRYPTED_RETAIL);

    /* A footer that differs in the first byte is not detected. */
    footer[0] = 'X';
    assert(!nand_detect_unlaunch_footer(footer));

    /* An unreadable MBR with no footer cannot be proven safe. */
    assert(nand_classify_image(NULL, NULL) == NAND_IMAGE_UNKNOWN);

    assert(nand_image_kind_text(NAND_IMAGE_DECRYPTED_SANDBOX) != NULL);
    assert(nand_image_kind_text(NAND_IMAGE_ENCRYPTED_RETAIL) != NULL);
    assert(nand_image_kind_text(NAND_IMAGE_UNKNOWN) != NULL);
    assert(nand_image_kind_text((nand_image_kind_t)99) != NULL);

    /* ---- retail raw-write safety floor ---- */
    /* Inside the bootsafe window. */
    assert(nand_retail_region_is_safe(NAND_RETAIL_BOOT_GUARD_SECTORS, 1u));
    assert(nand_retail_region_is_safe(0x1000u, 64u));
    assert(nand_retail_region_is_safe(NAND_RETAIL_USABLE_SECTORS - 1u, 1u));
    /* Any overlap with the boot chain is refused. */
    assert(!nand_retail_region_is_safe(0u, 1u));
    assert(!nand_retail_region_is_safe(NAND_RETAIL_BOOT_GUARD_SECTORS - 1u, 1u));
    assert(!nand_retail_region_is_safe(NAND_RETAIL_BOOT_GUARD_SECTORS - 1u,
                                       4u));
    /* Past the end of the chip / degenerate. */
    assert(!nand_retail_region_is_safe(NAND_RETAIL_USABLE_SECTORS, 1u));
    assert(!nand_retail_region_is_safe(NAND_RETAIL_USABLE_SECTORS - 1u, 2u));
    assert(!nand_retail_region_is_safe(NAND_RETAIL_USABLE_SECTORS, 0u));
    assert(!nand_retail_region_is_safe(0x1000u, 0u));
    assert(!nand_retail_region_is_safe(0xFFFFFFFFu, 0x20u));
}

/* ------------------------------------------------------------------ */
/* Deep FAT path resolution (against build/sdcard.img, optional)       */
/* ------------------------------------------------------------------ */

static u8 *g_img;
static u32 g_img_sectors;

static bool img_read(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    (void)ctx;

    if (lba >= g_img_sectors) {
        return false;
    }
    memcpy(out, g_img + (size_t)lba * FAT_SECTOR_SIZE, FAT_SECTOR_SIZE);
    return true;
}

static void test_resolve(const char *path)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    FILE *fh;
    long size;

    fh = fopen(path, "rb");
    if (fh == NULL) {
        printf("test_nand: skipping resolve test (%s not found)\n", path);
        return;
    }
    fseek(fh, 0L, SEEK_END);
    size = ftell(fh);
    fseek(fh, 0L, SEEK_SET);
    assert(size > 0);
    g_img = (u8 *)malloc((size_t)size);
    assert(g_img != NULL);
    assert(fread(g_img, 1u, (size_t)size, fh) == (size_t)size);
    fclose(fh);
    g_img_sectors = (u32)size / FAT_SECTOR_SIZE;

    assert(fat_mount(&vol, img_read, NULL));

    /* Root file. */
    assert(nand_resolve_path(&vol, "HELLO.NDS", &e));
    assert(!e.is_dir);

    /* Two-level path inside /NDS. */
    assert(nand_resolve_path(&vol, "NDS/SUBAPP.NDS", &e));
    assert(!e.is_dir);
    assert(nand_resolve_path(&vol, "/NDS/SUBAPP.NDS", &e));
    assert(!e.is_dir);

    /* A directory component resolves as a directory. */
    assert(nand_resolve_path(&vol, "NDS", &e));
    assert(e.is_dir);

    /* A deep path that does not exist fails closed. */
    assert(!nand_resolve_path(&vol, "title/00030004/484e4c41/content/00000000.app",
                              &e));
    assert(!nand_resolve_path(&vol, "", &e));
    assert(!nand_resolve_path(&vol, "/", &e));
    assert(!nand_resolve_path(NULL, "HELLO.NDS", &e));
    assert(!nand_resolve_path(&vol, "HELLO.NDS", NULL));

    free(g_img);
    g_img = NULL;
}

int main(int argc, char **argv)
{
    test_is_fat();
    test_pc_mbr();
    test_pc_mbr_no_signature();
    test_twl_table();
    test_policy();
    test_image_classify();

    if (argc > 1) {
        test_resolve(argv[1]);
    }

    puts("test_nand: all assertions passed");
    return 0;
}
