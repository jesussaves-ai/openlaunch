/*
 * tests/test_loader.c
 *
 * Host-side unit tests for the chainloader safety gate and the bounded
 * cluster-chain reader:
 *
 *   - nds_header_validate_loadable()  (src/common/nds_header.c)
 *   - fat_read_file_range()           (src/common/fat_ro.c)
 *   - loader_validate_file()          (src/arm9/loader.c)
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "common/fat_ro.h"
#include "common/nds_header.h"
#include "arm9/loader.h"

/* ------------------------------------------------------------------ */
/* Little-endian helpers + NDS header synthesis                        */
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

typedef struct {
    u32 a9_off, a9_entry, a9_ram, a9_size;
    u32 a7_off, a7_entry, a7_ram, a7_size;
    u8  unit;
} hdr_spec_t;

static void build_nds_header(u8 hdr[NDS_HEADER_SECTOR_SIZE],
                             const hdr_spec_t *s)
{
    memset(hdr, 0, NDS_HEADER_SECTOR_SIZE);
    memcpy(hdr + NDS_TITLE_OFF, "TEST GAME", 9);
    memcpy(hdr + NDS_GAMECODE_OFF, "TEST", 4);
    hdr[NDS_UNIT_CODE_OFF] = s->unit;

    put32(hdr, NDS_ARM9_ROM_OFFSET_OFF, s->a9_off);
    put32(hdr, NDS_ARM9_ENTRY_OFF,      s->a9_entry);
    put32(hdr, NDS_ARM9_RAM_ADDR_OFF,   s->a9_ram);
    put32(hdr, NDS_ARM9_SIZE_OFF,       s->a9_size);
    put32(hdr, NDS_ARM7_ROM_OFFSET_OFF, s->a7_off);
    put32(hdr, NDS_ARM7_ENTRY_OFF,      s->a7_entry);
    put32(hdr, NDS_ARM7_RAM_ADDR_OFF,   s->a7_ram);
    put32(hdr, NDS_ARM7_SIZE_OFF,       s->a7_size);

    put16(hdr, NDS_HEADER_CRC_OFF, nds_crc16(hdr, NDS_HEADER_CRC_LEN));
}

/* A known-good homebrew-style descriptor: ARM9 @ 0x02000000, ARM7 @ 0x02380000. */
static const hdr_spec_t k_valid_spec = {
    0x00004000u, 0x02000200u, 0x02000000u, 0x00004000u,
    0x00008000u, 0x02380000u, 0x02380000u, 0x00002000u,
    NDS_UNIT_CODE_NDS_DSI
};

/* ------------------------------------------------------------------ */
/* Header validation                                                   */
/* ------------------------------------------------------------------ */

static void test_bounds_default(void)
{
    nds_load_bounds_t b;

    nds_load_bounds_default(&b, false);
    assert(b.ram_base == NDS_SAFE_RAM_BASE);
    assert(b.ram_end == NDS_SAFE_RAM_END_DS);
    assert(b.arm7_wram_base == NDS_ARM7_WRAM_BASE);
    assert(b.arm7_wram_end == NDS_ARM7_WRAM_END);
    assert(b.reserve_base == NDS_BOOTLOADER_RESERVE_BASE);
    assert(b.reserve_end == NDS_BOOTLOADER_RESERVE_END);

    nds_load_bounds_default(&b, true);
    assert(b.ram_end == NDS_SAFE_RAM_END_TWL);

    nds_load_bounds_default(NULL, true);   /* must not crash */
}

static void test_validate_ok(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    nds_header_t h;
    nds_load_status_t st;

    build_nds_header(hdr, &k_valid_spec);
    st = nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u, NULL, &h);
    assert(st == NDS_LOAD_OK);
    assert(h.crc_ok);
    assert(h.arm9_ram_addr == 0x02000000u);
    assert(h.arm9_size == 0x00004000u);
    assert(h.arm7_ram_addr == 0x02380000u);
    assert(strcmp(h.title, "TEST GAME") == 0);
    assert(strcmp(h.gamecode, "TEST") == 0);
    assert(h.twl_capable);

    /* The same image also validates in the extended TWL window. */
    st = nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u, NULL, NULL);
    assert(st == NDS_LOAD_OK);
}

static void test_validate_bad_crc(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    nds_load_status_t st;

    build_nds_header(hdr, &k_valid_spec);
    hdr[0x100] ^= 0xFFu;   /* a covered byte, but not the stored CRC */
    st = nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u, NULL, NULL);
    assert(st == NDS_ERR_BAD_CRC);

    /* A wrong stored CRC is equally rejected. */
    build_nds_header(hdr, &k_valid_spec);
    put16(hdr, NDS_HEADER_CRC_OFF,
          (u16)(nds_crc16(hdr, NDS_HEADER_CRC_LEN) ^ 0x1234u));
    st = nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u, NULL, NULL);
    assert(st == NDS_ERR_BAD_CRC);
}

static void test_validate_arm9_bounds(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    hdr_spec_t s;
    nds_load_bounds_t b;

    nds_load_bounds_default(&b, false);

    /* Below Main RAM. */
    s = k_valid_spec;
    s.a9_ram = 0x01F00000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_ARM9_BOUNDS);

    /* Beyond the 4 MiB DS window. */
    s = k_valid_spec;
    s.a9_ram = 0x023C0000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_ARM9_BOUNDS);

    /* Zero size. */
    s = k_valid_spec;
    s.a9_size = 0u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_ARM9_BOUNDS);

    /* Unaligned load address. */
    s = k_valid_spec;
    s.a9_ram = 0x02000002u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_ARM9_BOUNDS);

    /* Entry outside the loaded range. */
    s = k_valid_spec;
    s.a9_entry = 0x02008000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_ARM9_BOUNDS);
}

static void test_validate_overlap_bootloader(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    hdr_spec_t s;
    nds_load_bounds_t b;

    nds_load_bounds_default(&b, false);

    /* ARM9 load range inside the loader reservation. */
    s = k_valid_spec;
    s.a9_ram = NDS_BOOTLOADER_RESERVE_BASE;
    s.a9_entry = NDS_BOOTLOADER_RESERVE_BASE + 0x100u;
    s.a9_size = 0x1000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_OVERLAP_BOOTLOADER);

    /* ARM7 load range inside the same reservation (Main-RAM target). */
    s = k_valid_spec;
    s.a9_ram = 0x02000000u;
    s.a7_ram = 0x02180000u;
    s.a7_entry = 0x02180000u;
    s.a7_size = 0x1000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        &b, NULL) == NDS_ERR_OVERLAP_BOOTLOADER);
}

static void test_validate_arm7_bounds(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    hdr_spec_t s;

    /* In DS mode 0x02900000 is outside the 4 MiB window. */
    s = k_valid_spec;
    s.a7_ram = 0x02900000u;
    s.a7_entry = 0x02900000u;
    s.a7_size = 0x4000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        NULL, NULL) == NDS_ERR_ARM7_BOUNDS);

    /* ...but valid in the 16 MiB TWL window. */
    {
        nds_load_bounds_t b;

        nds_load_bounds_default(&b, true);
        assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                            &b, NULL) == NDS_LOAD_OK);
    }

    /* ARM7-private WRAM is a valid target. */
    s = k_valid_spec;
    s.a7_ram = 0x037F8000u;
    s.a7_entry = 0x037F8000u;
    s.a7_size = 0x00008000u;
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        NULL, NULL) == NDS_LOAD_OK);

    /* A range that overruns the WRAM stack guard is rejected. */
    s = k_valid_spec;
    s.a7_ram = 0x0380F000u;
    s.a7_entry = 0x0380F000u;
    s.a7_size = 0x00004000u;   /* ends at 0x03813000 > 0x0380FFC0 */
    build_nds_header(hdr, &s);
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x20000u,
                                        NULL, NULL) == NDS_ERR_ARM7_BOUNDS);
}

static void test_validate_truncated(void)
{
    u8 hdr[NDS_HEADER_SECTOR_SIZE];

    build_nds_header(hdr, &k_valid_spec);

    /* Header buffer too short to be a sector. */
    assert(nds_header_validate_loadable(hdr, 100u, 0x20000u, NULL, NULL)
           == NDS_ERR_FILE_TRUNCATED);
    assert(nds_header_validate_loadable(NULL, sizeof(hdr), 0x20000u, NULL, NULL)
           == NDS_ERR_FILE_TRUNCATED);

    /* File smaller than a header sector. */
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 100u, NULL, NULL)
           == NDS_ERR_FILE_TRUNCATED);

    /* File too small to hold the ARM9 payload (off 0x4000 + size 0x4000). */
    assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x4000u, NULL, NULL)
           == NDS_ERR_FILE_TRUNCATED);

    /* ARM7 payload past EOF. */
    {
        hdr_spec_t s = k_valid_spec;

        s.a7_off = 0x00010000u;
        s.a7_size = 0x00002000u;   /* ends at 0x12000 > file size 0x11000 */
        build_nds_header(hdr, &s);
        assert(nds_header_validate_loadable(hdr, sizeof(hdr), 0x11000u,
                                            NULL, NULL) == NDS_ERR_FILE_TRUNCATED);
    }
}

static void test_status_text(void)
{
    assert(nds_load_status_text(NDS_LOAD_OK) != NULL);
    assert(nds_load_status_text(NDS_ERR_BAD_CRC) != NULL);
    assert(nds_load_status_text(NDS_ERR_ARM9_BOUNDS) != NULL);
    assert(nds_load_status_text(NDS_ERR_ARM7_BOUNDS) != NULL);
    assert(nds_load_status_text(NDS_ERR_OVERLAP_BOOTLOADER) != NULL);
    assert(nds_load_status_text(NDS_ERR_FILE_TRUNCATED) != NULL);
    assert(nds_load_status_text((nds_load_status_t)99) != NULL);
    assert(fat_chain_status_text(FAT_CHAIN_OK) != NULL);
    assert(fat_chain_status_text(FAT_CHAIN_ERR_LOOP) != NULL);
}

/* ------------------------------------------------------------------ */
/* Carrier volumes for the cluster-chain reader                        */
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

/* Deterministic data pattern for a data cluster. */
static u8 cluster_byte(u32 cluster, u32 offset)
{
    return (u8)(((cluster - 2u) * 31u + offset * 7u + 1u) & 0xFFu);
}

/* FAT16 bad-cluster marker (0xFFF7). */
#define FAT16_BAD_VALUE 0xFFF7u

static void fill_cluster(u32 base_lba, u32 data_start, u32 cluster)
{
    u32 j;
    u8 *cl = g_img + (base_lba + data_start + (cluster - 2u)) * FAT_SECTOR_SIZE;

    for (j = 0u; j < FAT_SECTOR_SIZE; j++) {
        cl[j] = cluster_byte(cluster, j);
    }
}

/* FAT16: 1 sector/cluster, 1 reserved, 1 FAT, 16 root entries (1 sector). */
#define F16_RESERVED 1u
#define F16_FAT_SIZE 1u
#define F16_ROOT_SEC 1u
#define F16_DATA     (F16_RESERVED + F16_FAT_SIZE + F16_ROOT_SEC)   /* 3 */
#define F16_CLUSTERS 24u
#define F16_TOTAL    (F16_DATA + F16_CLUSTERS)

static u8 *f16_fat(void)  { return g_img + F16_RESERVED * FAT_SECTOR_SIZE; }
static u8 *f16_root(void) { return g_img + (F16_RESERVED + F16_FAT_SIZE) * FAT_SECTOR_SIZE; }

static void f16_set(u32 cluster, u32 value)
{
    put16(f16_fat(), cluster * 2u, (u16)value);
}

static void build_fat16_volume(void)
{
    u8 *bpb = g_img;
    u8 *root;
    u32 c;

    memset(g_img, 0, sizeof(g_img));
    g_img_sectors = F16_TOTAL;

    bpb[0] = 0xEB; bpb[1] = 0x3C; bpb[2] = 0x90;
    memcpy(bpb + 3, "MSDOS5.0", 8);
    put16(bpb, 11, 512);
    bpb[13] = 1;                       /* sectors per cluster */
    put16(bpb, 14, F16_RESERVED);
    bpb[16] = 1;                       /* number of FATs */
    put16(bpb, 17, 16);                /* root entries */
    put16(bpb, 19, F16_TOTAL);
    bpb[21] = 0xF8;
    put16(bpb, 22, F16_FAT_SIZE);
    memcpy(bpb + 54, "FAT16   ", 8);
    bpb[510] = 0x55; bpb[511] = 0xAA;

    /* Media descriptor + reserved entry. */
    put16(f16_fat(), 0, 0xFFF8);
    put16(f16_fat(), 2, 0xFFFF);

    for (c = 2u; c < 2u + F16_CLUSTERS; c++) {
        fill_cluster(0u, F16_DATA, c);
    }

    /* A fragmented 4-cluster chain: 3 -> 6 -> 7 -> 4 (EOC). */
    f16_set(3u, 6u);
    f16_set(6u, 7u);
    f16_set(7u, 4u);
    f16_set(4u, 0xFFFFu);

    root = f16_root();
    /* FRAG.NDS: 3 clusters chained through 4. */
    memcpy(root + 0u, "FRAG    ", 8);
    memcpy(root + 8u, "NDS", 3);
    root[11] = 0x20;
    put16(root, 26u, 3u);
    put32(root, 28u, 2048u);
}

/*
 * FAT16 with 4 sectors per cluster (2048-byte clusters). Used to exercise a
 * read whose byte offset is not sector-aligned and which spans several sectors
 * within one cluster.
 */
#define F16B_SPC      4u
#define F16B_RESERVED 1u
#define F16B_FAT_SIZE 1u
#define F16B_ROOT_SEC 1u
#define F16B_DATA     (F16B_RESERVED + F16B_FAT_SIZE + F16B_ROOT_SEC)
#define F16B_CLUSTERS 8u
#define F16B_TOTAL    (F16B_DATA + F16B_CLUSTERS * F16B_SPC)

static u8 *f16b_fat(void) { return g_img + F16B_RESERVED * FAT_SECTOR_SIZE; }

static void f16b_set(u32 cluster, u32 value)
{
    put16(f16b_fat(), cluster * 2u, (u16)value);
}

static void build_fat16_wide_volume(void)
{
    u8 *bpb = g_img;
    u32 c;
    u32 sec;

    memset(g_img, 0, sizeof(g_img));
    g_img_sectors = F16B_TOTAL;

    bpb[0] = 0xEB; bpb[1] = 0x3C; bpb[2] = 0x90;
    memcpy(bpb + 3, "MSDOS5.0", 8);
    put16(bpb, 11, 512);
    bpb[13] = F16B_SPC;
    put16(bpb, 14, F16B_RESERVED);
    bpb[16] = 1;
    put16(bpb, 17, 16);
    put16(bpb, 19, F16B_TOTAL);
    bpb[21] = 0xF8;
    put16(bpb, 22, F16B_FAT_SIZE);
    memcpy(bpb + 54, "FAT16   ", 8);
    bpb[510] = 0x55; bpb[511] = 0xAA;

    put16(f16b_fat(), 0, 0xFFF8);
    put16(f16b_fat(), 2, 0xFFFF);

    for (c = 2u; c < 2u + F16B_CLUSTERS; c++) {
        for (sec = 0u; sec < F16B_SPC; sec++) {
            u32 j;
            u8 *cl = g_img + (F16B_DATA + (c - 2u) * F16B_SPC + sec) *
                              FAT_SECTOR_SIZE;

            for (j = 0u; j < FAT_SECTOR_SIZE; j++) {
                cl[j] = cluster_byte(c, sec * FAT_SECTOR_SIZE + j);
            }
        }
    }

    /* Fragmented chain: cluster 3 -> 5 (EOC). */
    f16b_set(3u, 5u);
    f16b_set(5u, 0xFFFFu);
}

/* FAT32: 1 sector/cluster, 32 reserved, 1 FAT, root cluster 2. */
#define F32_RESERVED 32u
#define F32_FAT_SIZE 1u
#define F32_DATA     (F32_RESERVED + F32_FAT_SIZE)   /* 33 */
#define F32_CLUSTERS 24u
#define F32_TOTAL    (F32_DATA + F32_CLUSTERS)

static u8 *f32_fat(void) { return g_img + F32_RESERVED * FAT_SECTOR_SIZE; }
static u8 *f32_root(void) { return g_img + F32_DATA * FAT_SECTOR_SIZE; }

static void f32_set(u32 cluster, u32 value)
{
    put32(f32_fat(), cluster * 4u, value & 0x0FFFFFFFu);
}

static void build_fat32_volume(void)
{
    u8 *bpb = g_img;
    u32 c;

    memset(g_img, 0, sizeof(g_img));
    g_img_sectors = F32_TOTAL;

    bpb[0] = 0xEB; bpb[1] = 0x58; bpb[2] = 0x90;
    memcpy(bpb + 3, "MSWIN4.1", 8);
    put16(bpb, 11, 512);
    bpb[13] = 1;
    put16(bpb, 14, F32_RESERVED);
    bpb[16] = 1;
    put16(bpb, 17, 0);
    put16(bpb, 19, 0);
    bpb[21] = 0xF8;
    put16(bpb, 22, 0);
    put32(bpb, 32, F32_TOTAL);
    put32(bpb, 36, F32_FAT_SIZE);
    put32(bpb, 44, 2u);                /* root cluster */
    memcpy(bpb + 54, "FAT32   ", 8);
    bpb[510] = 0x55; bpb[511] = 0xAA;

    put32(f32_fat(), 0, 0x0FFFFFF8u);
    put32(f32_fat(), 4, 0x0FFFFFFFu);

    for (c = 2u; c < 2u + F32_CLUSTERS; c++) {
        fill_cluster(0u, F32_DATA, c);
    }

    f32_set(2u, 0x0FFFFFFFu);          /* root EOC    */
    f32_set(3u, 4u);                   /* file chain  */
    f32_set(4u, 0x0FFFFFFFu);

    memcpy(f32_root() + 0u, "ROOT    ", 8);
    memcpy(f32_root() + 8u, "SRL", 3);
    f32_root()[11] = 0x20;
    put16(f32_root(), 20u, 0u);
    put16(f32_root(), 26u, 3u);
    put32(f32_root(), 28u, 700u);
}

/* ------------------------------------------------------------------ */
/* Cluster-chain reader                                                */
/* ------------------------------------------------------------------ */

static void test_fat16_range(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[2048];
    u32 got = 0u;
    fat_chain_status_t cs = FAT_CHAIN_ERR_ARG;
    u32 i;

    build_fat16_volume();
    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT16);

    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 2048u;

    /* Full fragmented read: 3 -> 6 -> 7 -> 4. */
    assert(fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_OK);
    assert(got == 2048u);
    assert(buf[0] == cluster_byte(3u, 0u));
    assert(buf[512] == cluster_byte(6u, 0u));
    assert(buf[1024] == cluster_byte(7u, 0u));
    assert(buf[1536] == cluster_byte(4u, 0u));

    /* Offset into the middle of cluster 6. */
    assert(fat_read_file_range(&vol, &e, 600u, buf, 200u, &got, &cs));
    assert(cs == FAT_CHAIN_OK);
    assert(got == 200u);
    for (i = 0u; i < 200u; i++) {
        assert(buf[i] == cluster_byte(6u, (600u - 512u) + i));
    }

    /* Clamp at EOF. */
    assert(fat_read_file_range(&vol, &e, 2000u, buf, 512u, &got, &cs));
    assert(cs == FAT_CHAIN_OK);
    assert(got == 48u);

    /* Zero-byte read at EOF is OK; past EOF is truncated. */
    assert(fat_read_file_range(&vol, &e, 2048u, buf, 16u, &got, &cs));
    assert(got == 0u && cs == FAT_CHAIN_OK);
    assert(!fat_read_file_range(&vol, &e, 2049u, buf, 16u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_TRUNCATED);
}

static void test_fat16_wide_cluster(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[2500];
    u32 got = 0u;
    fat_chain_status_t cs = FAT_CHAIN_ERR_ARG;
    u32 i;

    build_fat16_wide_volume();
    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT16);
    assert(vol.sectors_per_cluster == F16B_SPC);

    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 3000u;

    /* Offset 100 is not sector-aligned; 2500 bytes span all four sectors of
     * cluster 3 and into cluster 5. */
    assert(fat_read_file_range(&vol, &e, 100u, buf, 2500u, &got, &cs));
    assert(cs == FAT_CHAIN_OK && got == 2500u);
    for (i = 0u; i < 1948u; i++) {
        assert(buf[i] == cluster_byte(3u, 100u + i));
    }
    for (i = 0u; i < 552u; i++) {
        assert(buf[1948u + i] == cluster_byte(5u, i));
    }
}

static void test_fat_chain_guards(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[2048];
    u32 got = 0u;
    fat_chain_status_t cs = FAT_CHAIN_ERR_ARG;

    /* Self-referential cluster => LOOP. */
    build_fat16_volume();
    f16_set(3u, 3u);
    assert(fat_mount(&vol, test_read, NULL));
    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 2048u;
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_LOOP);
    assert(got == 512u);

    /* Two-cluster ping-pong => LOOP. */
    build_fat16_volume();
    f16_set(3u, 6u);
    f16_set(6u, 3u);
    assert(fat_mount(&vol, test_read, NULL));
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_LOOP);

    /* Bad cluster marker => BAD_CLUSTER. */
    build_fat16_volume();
    f16_set(3u, FAT16_BAD_VALUE);
    assert(fat_mount(&vol, test_read, NULL));
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_BAD_CLUSTER);

    /* Free cluster (0) in the chain => BAD_CLUSTER. */
    build_fat16_volume();
    f16_set(3u, 0u);
    assert(fat_mount(&vol, test_read, NULL));
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_BAD_CLUSTER);

    /* Premature EOC => TRUNCATED. */
    build_fat16_volume();
    f16_set(3u, 0xFFFFu);
    assert(fat_mount(&vol, test_read, NULL));
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_TRUNCATED);
    assert(got == 512u);

    /* Invalid start cluster. */
    memset(&e, 0, sizeof(e));
    e.start_cluster = 0u;
    e.file_size = 512u;
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 512u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_BAD_CLUSTER);

    /* Failed sector read => IO error. */
    build_fat16_volume();
    assert(fat_mount(&vol, test_read, NULL));
    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 2048u;
    g_img_sectors = 0u;    /* every read now fails */
    assert(!fat_read_file_range(&vol, &e, 0u, buf, 2048u, &got, &cs));
    assert(cs == FAT_CHAIN_ERR_IO);
}

static void test_fat32_range(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[700];
    u32 got = 0u;
    fat_chain_status_t cs = FAT_CHAIN_ERR_ARG;

    build_fat32_volume();
    assert(fat_mount(&vol, test_read, NULL));
    assert(vol.type == FAT_TYPE_FAT32);

    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 700u;

    assert(fat_read_file_range(&vol, &e, 0u, buf, 700u, &got, &cs));
    assert(cs == FAT_CHAIN_OK && got == 700u);
    assert(buf[0] == cluster_byte(3u, 0u));
    assert(buf[512] == cluster_byte(4u, 0u));

    /* Read only the tail cluster. */
    assert(fat_read_file_range(&vol, &e, 512u, buf, 188u, &got, &cs));
    assert(cs == FAT_CHAIN_OK && got == 188u);
    assert(buf[0] == cluster_byte(4u, 0u));
}

/* ------------------------------------------------------------------ */
/* loader_validate_file() end to end over a synthetic volume           */
/* ------------------------------------------------------------------ */

static void test_loader_validate(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    nds_header_t h;
    nds_load_status_t st;
    u8 hdr[NDS_HEADER_SECTOR_SIZE];
    hdr_spec_t s = k_valid_spec;

    /* Put a valid header in cluster 3 of the FAT16 volume. */
    build_fat16_volume();
    build_nds_header(hdr, &s);
    {
        u8 *cl = g_img + (F16_DATA + (3u - 2u)) * FAT_SECTOR_SIZE;

        memcpy(cl, hdr, NDS_HEADER_SECTOR_SIZE);
    }
    assert(fat_mount(&vol, test_read, NULL));

    memset(&e, 0, sizeof(e));
    e.start_cluster = 3u;
    e.file_size = 0x20000u;

    st = loader_validate_file(&vol, &e, false, &h);
    assert(st == NDS_LOAD_OK);
    assert(h.arm9_entry == s.a9_entry);
    assert(h.arm7_entry == s.a7_entry);

    /* A too-small file is rejected as truncated. */
    e.file_size = 100u;
    assert(loader_validate_file(&vol, &e, false, &h) == NDS_ERR_FILE_TRUNCATED);

    /* A corrupt header (bad CRC) is rejected after being read. */
    e.file_size = 0x20000u;
    {
        u8 *cl = g_img + (F16_DATA + (3u - 2u)) * FAT_SECTOR_SIZE;

        cl[0x100] ^= 0xFFu;
    }
    assert(loader_validate_file(&vol, &e, false, &h) == NDS_ERR_BAD_CRC);
}

static void test_loader_plan(void)
{
    nds_header_t h;
    loader_plan_t plan;
    u8 hdr[NDS_HEADER_SECTOR_SIZE];

    build_nds_header(hdr, &k_valid_spec);
    assert(nds_header_parse(hdr, sizeof(hdr), &h));

    loader_plan_from_header(&h, true, &plan);
    assert(plan.twl_extended);
    assert(plan.arm7_bounce);
    assert(plan.arm7_stage_addr == LOADER_ARM7_BOUNCE_ADDR);

    loader_plan_from_header(NULL, false, &plan);   /* must not crash */
    assert(!plan.twl_extended);
    loader_plan_from_header(&h, false, NULL);       /* must not crash */
}

int main(void)
{
    test_bounds_default();
    test_validate_ok();
    test_validate_bad_crc();
    test_validate_arm9_bounds();
    test_validate_overlap_bootloader();
    test_validate_arm7_bounds();
    test_validate_truncated();
    test_status_text();

    test_fat16_range();
    test_fat16_wide_cluster();
    test_fat_chain_guards();
    test_fat32_range();

    test_loader_validate();
    test_loader_plan();

    puts("test_loader: all assertions passed");
    return 0;
}
