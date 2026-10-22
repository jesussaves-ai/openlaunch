/*
 * tests/test_fat_ro.c
 *
 * Host-side unit tests for the read-only FAT16/FAT32 parser
 * (src/common/fat_ro.c) and the NDS header validator
 * (src/common/nds_header.c).
 *
 * The tests synthesize complete in-memory volumes (MBR, BPB, FAT tables, root
 * directory and data clusters) and drive the parser through its public API with
 * a plain memory sector-read callback.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "common/fat_ro.h"
#include "common/nds_header.h"

/* ------------------------------------------------------------------ */
/* Tiny little-endian helpers                                          */
/* ------------------------------------------------------------------ */

static void put16(u8 *p, u32 off, u16 v)
{
    p[off] = (u8)(v & 0xFFu);
    p[off + 1u] = (u8)(v >> 8);
}

static void put32(u8 *p, u32 off, u32 v)
{
    p[off] = (u8)(v & 0xFFu);
    p[off + 1u] = (u8)((v >> 8) & 0xFFu);
    p[off + 2u] = (u8)((v >> 16) & 0xFFu);
    p[off + 3u] = (u8)((v >> 24) & 0xFFu);
}

/* Copy `s` into a fixed-width, space-padded FAT name field. */
static void set_field(u8 *p, u32 off, const char *s, u32 n)
{
    u32 i;

    for (i = 0u; i < n; i++) {
        p[off + i] = (s[i] != '\0') ? (u8)s[i] : (u8)' ';
    }
}

static void set_entry(u8 *dir, u32 idx, const char *base, const char *ext,
                      u8 attr, u32 cluster, u32 size)
{
    u8 *e = dir + idx * FAT_DIR_ENTRY_SIZE;

    memset(e, 0, FAT_DIR_ENTRY_SIZE);
    set_field(e, 0u, base, 8u);
    set_field(e, 8u, ext, 3u);
    e[11] = attr;
    put16(e, 20u, (u16)((cluster >> 16) & 0xFFFFu));
    put16(e, 26u, (u16)(cluster & 0xFFFFu));
    put32(e, 28u, size);
}

/* Deterministic data pattern for a data cluster. */
static u8 cluster_byte(u32 cluster, u32 offset)
{
    return (u8)(((cluster - 2u) * 31u + offset * 7u + 1u) & 0xFFu);
}

/* ------------------------------------------------------------------ */
/* In-memory image + read callback                                     */
/* ------------------------------------------------------------------ */

static u8  g_img[65536];
static u32 g_img_sectors;

static bool test_read(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    (void)ctx;

    if (lba >= g_img_sectors) {
        return false;
    }
    memcpy(out, g_img + lba * FAT_SECTOR_SIZE, FAT_SECTOR_SIZE);
    return true;
}

/* ---- FAT16 geometry (superfloppy at LBA 0) ---- */
#define F16_SPC          1u
#define F16_RESERVED     1u
#define F16_NFATS        1u
#define F16_ROOT_ENTRIES 16u
#define F16_FAT_SIZE     1u
#define F16_CLUSTERS     32u
#define F16_ROOT_SECTORS 1u
#define F16_DATA_START   (F16_RESERVED + F16_NFATS * F16_FAT_SIZE + F16_ROOT_SECTORS)
#define F16_TOTAL        (F16_DATA_START + F16_CLUSTERS * F16_SPC)

static void build_fat16(u8 *img, u32 base)
{
    u8 *bpb = img + base * FAT_SECTOR_SIZE;
    u8 *fat = img + (base + F16_RESERVED) * FAT_SECTOR_SIZE;
    u8 *root = img + (base + F16_RESERVED + F16_FAT_SIZE) * FAT_SECTOR_SIZE;
    u32 i;
    u32 j;

    memset(bpb, 0, FAT_SECTOR_SIZE);
    bpb[0] = 0xEB; bpb[1] = 0x3C; bpb[2] = 0x90;
    memcpy(bpb + 3, "MSDOS5.0", 8);
    put16(bpb, 11, 512);
    bpb[13] = F16_SPC;
    put16(bpb, 14, F16_RESERVED);
    bpb[16] = F16_NFATS;
    put16(bpb, 17, F16_ROOT_ENTRIES);
    put16(bpb, 19, F16_TOTAL);
    bpb[21] = 0xF8;
    put16(bpb, 22, F16_FAT_SIZE);
    put16(bpb, 24, 32);
    put16(bpb, 26, 2);
    put32(bpb, 28, base);
    put32(bpb, 32, 0);
    put32(bpb, 36, 0);
    put32(bpb, 44, 0);
    memcpy(bpb + 54, "FAT16   ", 8);
    bpb[510] = 0x55; bpb[511] = 0xAA;

    memset(fat, 0, FAT_SECTOR_SIZE);
    put16(fat, 0, 0xFFF8);       /* media descriptor */
    put16(fat, 2, 0xFFFF);
    put16(fat, 4, 0xFFFF);       /* cluster 2 EOC   */
    put16(fat, 6, 4);            /* cluster 3 -> 4  */
    put16(fat, 8, 0xFFFF);       /* cluster 4 EOC   */
    put16(fat, 10, 0xFFFF);      /* cluster 5 EOC   */
    put16(fat, 12, 0xFFFF);      /* cluster 6 EOC   */

    memset(root, 0, FAT_SECTOR_SIZE);
    set_entry(root, 0, "GAME", "NDS", 0x20, 2, 100);
    set_entry(root, 1, "BIG", "NDS", 0x20, 3, 600);
    root[2 * FAT_DIR_ENTRY_SIZE + 0] = 0x41;                 /* LFN entry   */
    root[2 * FAT_DIR_ENTRY_SIZE + 11] = 0x0F;
    set_entry(root, 3, "SUBDIR", "", 0x10, 0, 0);            /* directory   */
    set_entry(root, 4, "SAVE", "TXT", 0x20, 5, 10);
    root[5 * FAT_DIR_ENTRY_SIZE + 0] = 0xE5;                 /* deleted     */
    set_entry(root, 6, "VOLLABEL", "", 0x08, 0, 0);          /* volume id   */
    set_entry(root, 7, "SECOND", "SRL", 0x20, 6, 20);
    /* entry 8 stays 0x00 => end of directory */

    for (i = 0u; i < F16_CLUSTERS; i++) {
        u8 *cl = img + (base + F16_DATA_START + i) * FAT_SECTOR_SIZE;
        for (j = 0u; j < FAT_SECTOR_SIZE; j++) {
            cl[j] = cluster_byte(i + 2u, j);
        }
    }
}

static void build_mbr(u8 *img, u32 part_lba, u32 part_sectors, u8 type)
{
    memset(img, 0, FAT_SECTOR_SIZE);
    img[446 + 4] = type;
    put32(img, 446 + 8, part_lba);
    put32(img, 446 + 12, part_sectors);
    img[510] = 0x55;
    img[511] = 0xAA;
}

/* ---- FAT32 geometry ---- */
#define F32_SPC          1u
#define F32_RESERVED     32u
#define F32_NFATS        1u
#define F32_FAT_SIZE     1u
#define F32_CLUSTERS     32u
#define F32_DATA_START   (F32_RESERVED + F32_NFATS * F32_FAT_SIZE)
#define F32_TOTAL        (F32_DATA_START + F32_CLUSTERS * F32_SPC)

static void build_fat32(u8 *img, u32 base)
{
    u8 *bpb = img + base * FAT_SECTOR_SIZE;
    u8 *fat = img + (base + F32_RESERVED) * FAT_SECTOR_SIZE;
    u8 *root = img + (base + F32_DATA_START) * FAT_SECTOR_SIZE;  /* cluster 2 */
    u8 *file = img + (base + F32_DATA_START + 1u) * FAT_SECTOR_SIZE; /* cl 3 */
    u32 j;

    memset(bpb, 0, FAT_SECTOR_SIZE);
    bpb[0] = 0xEB; bpb[1] = 0x58; bpb[2] = 0x90;
    memcpy(bpb + 3, "MSWIN4.1", 8);
    put16(bpb, 11, 512);
    bpb[13] = F32_SPC;
    put16(bpb, 14, F32_RESERVED);
    bpb[16] = F32_NFATS;
    put16(bpb, 17, 0);
    put16(bpb, 19, 0);
    bpb[21] = 0xF8;
    put16(bpb, 22, 0);
    put32(bpb, 32, F32_TOTAL);
    put32(bpb, 36, F32_FAT_SIZE);
    put32(bpb, 44, 2);           /* root directory cluster */
    memcpy(bpb + 54, "FAT32   ", 8);
    bpb[510] = 0x55; bpb[511] = 0xAA;

    memset(fat, 0, FAT_SECTOR_SIZE);
    put32(fat, 0, 0x0FFFFFF8u);
    put32(fat, 4, 0x0FFFFFFFu);
    put32(fat, 8, 0x0FFFFFFFu);   /* cluster 2 (root) EOC */
    put32(fat, 12, 0x0FFFFFFFu);  /* cluster 3 EOC        */
    put32(fat, 16, 0x0FFFFFFFu);  /* cluster 4 EOC        */

    memset(root, 0, FAT_SECTOR_SIZE);
    set_entry(root, 0, "ROOT", "NDS", 0x20, 3, 10);

    for (j = 0u; j < FAT_SECTOR_SIZE; j++) {
        file[j] = cluster_byte(3u, j);
    }
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_fat16_mount_and_list(void)
{
    fat_volume_t vol;
    fat_dir_iter_t it;
    fat_file_entry_t e;
    u32 n;

    memset(g_img, 0, sizeof(g_img));
    build_fat16(g_img, 0u);
    g_img_sectors = F16_TOTAL;

    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT16);
    assert(vol.partition_lba == 0u);
    assert(vol.bytes_per_sector == 512u);
    assert(vol.sectors_per_cluster == F16_SPC);
    assert(vol.fat_start_lba == F16_RESERVED);
    assert(vol.root_start_lba == F16_RESERVED + F16_FAT_SIZE);
    assert(vol.data_start_lba == F16_DATA_START);
    assert(vol.cluster_count == F16_CLUSTERS);
    assert(vol.max_cluster == F16_CLUSTERS + 1u);

    /* .nds/.srl filter: GAME.NDS, BIG.NDS, SECOND.SRL. */
    fat_dir_iter_init(&vol, &it, true);
    n = 0u;
    while (fat_dir_next(&it, &e)) {
        assert(n < 3u);
        if (n == 0u) {
            assert(strcmp(e.name, "GAME.NDS") == 0);
            assert(e.start_cluster == 2u);
            assert(e.file_size == 100u);
        } else if (n == 1u) {
            assert(strcmp(e.name, "BIG.NDS") == 0);
            assert(e.start_cluster == 3u);
            assert(e.file_size == 600u);
        } else {
            assert(strcmp(e.name, "SECOND.SRL") == 0);
            assert(e.start_cluster == 6u);
            assert(e.file_size == 20u);
        }
        n++;
    }
    assert(n == 3u);

    /* Unfiltered listing also sees SAVE.TXT (and skips LFN/dir/volume). */
    fat_dir_iter_init(&vol, &it, false);
    n = 0u;
    while (fat_dir_next(&it, &e)) {
        n++;
    }
    assert(n == 4u);
}

static void test_fat16_chain_and_read(void)
{
    fat_volume_t vol;
    u8 buf[1024];
    u32 got;
    u32 i;

    memset(g_img, 0, sizeof(g_img));
    build_fat16(g_img, 0u);
    g_img_sectors = F16_TOTAL;
    assert(fat_mount(&vol, test_read, NULL));

    /* Chain traversal. */
    assert(fat_get_next_cluster(&vol, 3u) == 4u);
    assert(fat_get_next_cluster(&vol, 4u) == 0xFFFFu);
    assert(fat_is_eoc(&vol, 0xFFFFu));
    assert(!fat_is_eoc(&vol, 3u));
    assert(fat_get_next_cluster(&vol, 0u) == 0xFFFFFFFFu);

    /* Single-cluster read. */
    {
        fat_file_entry_t e;
        memset(&e, 0, sizeof(e));
        e.start_cluster = 2u;
        e.file_size = 100u;
        assert(fat_read_file(&vol, &e, buf, sizeof(buf), &got));
        assert(got == 100u);
        for (i = 0u; i < 100u; i++) {
            assert(buf[i] == cluster_byte(2u, i));
        }
    }

    /* Two-cluster read crossing the cluster boundary. */
    {
        fat_file_entry_t e;
        memset(&e, 0, sizeof(e));
        e.start_cluster = 3u;
        e.file_size = 600u;
        assert(fat_read_file(&vol, &e, buf, sizeof(buf), &got));
        assert(got == 600u);
        for (i = 0u; i < 512u; i++) {
            assert(buf[i] == cluster_byte(3u, i));
        }
        for (i = 0u; i < 88u; i++) {
            assert(buf[512u + i] == cluster_byte(4u, i));
        }
    }

    /* max_len clamps the transfer. */
    {
        fat_file_entry_t e;
        memset(&e, 0, sizeof(e));
        e.start_cluster = 2u;
        e.file_size = 100u;
        assert(fat_read_file(&vol, &e, buf, 50u, &got));
        assert(got == 50u);
    }
}

static void test_fat16_truncated_chain(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[1024];
    u32 got = 123u;

    memset(g_img, 0, sizeof(g_img));
    build_fat16(g_img, 0u);
    g_img_sectors = F16_TOTAL;
    assert(fat_mount(&vol, test_read, NULL));

    /* Claims 2000 bytes but cluster 2 is an immediate EOC. */
    memset(&e, 0, sizeof(e));
    e.start_cluster = 2u;
    e.file_size = 2000u;
    assert(!fat_read_file(&vol, &e, buf, sizeof(buf), &got));
    assert(got == 512u);
}

static void test_fat16_mbr(void)
{
    fat_volume_t vol;
    fat_dir_iter_t it;
    fat_file_entry_t e;
    u32 n = 0u;
    const u32 part_lba = 4u;

    memset(g_img, 0, sizeof(g_img));
    build_mbr(g_img, part_lba, F16_TOTAL, FAT_PART_TYPE_FAT16);
    build_fat16(g_img, part_lba);
    g_img_sectors = part_lba + F16_TOTAL;

    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT16);
    assert(vol.partition_lba == part_lba);
    assert(vol.fat_start_lba == part_lba + F16_RESERVED);
    assert(vol.data_start_lba == part_lba + F16_DATA_START);

    fat_dir_iter_init(&vol, &it, true);
    while (fat_dir_next(&it, &e)) {
        n++;
    }
    assert(n == 3u);
}

static void test_fat32(void)
{
    fat_volume_t vol;
    fat_dir_iter_t it;
    fat_file_entry_t e;
    u8 buf[512];
    u32 got;
    u32 n = 0u;

    memset(g_img, 0, sizeof(g_img));
    build_fat32(g_img, 0u);
    g_img_sectors = F32_TOTAL;

    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT32);
    assert(vol.root_cluster == 2u);
    assert(vol.root_entry_count == 0u);
    assert(vol.fat_start_lba == F32_RESERVED);
    assert(vol.data_start_lba == F32_DATA_START);

    assert(fat_get_next_cluster(&vol, 2u) == 0x0FFFFFFFu);
    assert(fat_is_eoc(&vol, 0x0FFFFFFFu));
    assert(!fat_is_eoc(&vol, 3u));

    fat_dir_iter_init(&vol, &it, true);
    while (fat_dir_next(&it, &e)) {
        assert(strcmp(e.name, "ROOT.NDS") == 0);
        assert(e.start_cluster == 3u);
        assert(e.file_size == 10u);
        n++;
    }
    assert(n == 1u);

    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 10u;
    assert(fat_read_file(&vol, &e, buf, sizeof(buf), &got));
    assert(got == 10u);
    assert(buf[0] == cluster_byte(3u, 0u));
}

static void test_mount_failures(void)
{
    fat_volume_t vol;

    /* All-zero media: no signature, no valid BPB. */
    memset(g_img, 0, sizeof(g_img));
    g_img_sectors = 8u;
    assert(!fat_mount(&vol, test_read, NULL));

    /* A read past the end of the medium must fail closed. */
    g_img_sectors = 0u;
    assert(!fat_mount(&vol, test_read, NULL));

    assert(!fat_mount(NULL, test_read, NULL));
    assert(!fat_mount(&vol, NULL, NULL));
}

static void test_nds_filter(void)
{
    fat_file_entry_t e;

    memset(&e, 0, sizeof(e));
    strcpy(e.name, "GAME.NDS");
    assert(fat_is_nds_entry(&e));
    strcpy(e.name, "GAME.SRL");
    assert(fat_is_nds_entry(&e));
    strcpy(e.name, "game.nds");
    assert(fat_is_nds_entry(&e));
    strcpy(e.name, "SAVE.TXT");
    assert(!fat_is_nds_entry(&e));
    strcpy(e.name, "NOEXT");
    assert(!fat_is_nds_entry(&e));
    assert(!fat_is_nds_entry(NULL));
}

static void test_nds_header(void)
{
    u8 hdr[512];
    nds_header_t h;
    u16 crc;

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr + NDS_TITLE_OFF, "TEST GAME", 9);
    memcpy(hdr + NDS_GAMECODE_OFF, "TEST", 4);
    crc = nds_crc16(hdr, NDS_HEADER_CRC_LEN);
    put16(hdr, NDS_HEADER_CRC_OFF, crc);

    assert(nds_header_parse(hdr, sizeof(hdr), &h));
    assert(strcmp(h.title, "TEST GAME") == 0);
    assert(strcmp(h.gamecode, "TEST") == 0);
    assert(h.crc_ok);
    assert(h.stored_crc == h.computed_crc);

    /* A single flipped byte invalidates the header CRC. */
    hdr[0x100] ^= 0xFFu;
    assert(nds_header_parse(hdr, sizeof(hdr), &h));
    assert(!h.crc_ok);

    /* Too short. */
    assert(!nds_header_parse(hdr, 16u, &h));
    assert(!nds_header_parse(NULL, sizeof(hdr), &h));
}

int main(void)
{
    test_fat16_mount_and_list();
    test_fat16_chain_and_read();
    test_fat16_truncated_chain();
    test_fat16_mbr();
    test_fat32();
    test_mount_failures();
    test_nds_filter();
    test_nds_header();

    puts("test_fat_ro: all assertions passed");
    return 0;
}
