/*
 * tests/test_sd_image.c
 *
 * End-to-end host integration test for the chainload SD pipeline. It mounts the
 * deterministic image produced by scripts/make_sd_image.py (build/sdcard.img)
 * and drives the real production functions in order:
 *
 *   fat_mbr_find_fat_partition() -> fat_mount() -> fat_dir_iter_*()
 *       -> loader_validate_file() -> loader_stage_and_boot() (dry run)
 *
 * and asserts:
 *   - HELLO.NDS is listed, validates as NDS_LOAD_OK and stages cleanly;
 *   - BADCRC.NDS is listed and rejected with NDS_ERR_BAD_CRC;
 *   - OVERLAP.NDS is listed and rejected with NDS_ERR_OVERLAP_BOOTLOADER;
 *   - README.TXT is hidden by the .nds/.srl browser filter.
 *
 * src/arm9/loader.c is compiled with -DLOADER_HOST_TEST: that redirects the
 * absolute-address payload writes into the scratch arenas below and reports the
 * IPC handoff request instead of touching hardware. The production build never
 * defines it.
 *
 * Build/run with:  make host-test   (which first runs `make sd-image`)
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/fat_ro.h"
#include "common/nds_header.h"
#include "arm9/loader.h"

/* ------------------------------------------------------------------ */
/* LOADER_HOST_TEST staging hooks (see src/arm9/loader.c)              */
/* ------------------------------------------------------------------ */

#define ARENA_BYTES (1u << 20)   /* 1 MiB, matches LOADER_ARM7_BOUNCE_MAX */

static u8  s_arm9_arena[ARENA_BYTES];
static u8  s_arm7_arena[ARENA_BYTES];
static u8  s_header_arena[FAT_SECTOR_SIZE];
static u32 s_handoff_calls;
static u32 s_handoff_args[4];

u8 *loader_host_test_stage(u32 addr, u32 len)
{
    (void)len;

    if (addr == LOADER_ARM7_BOUNCE_ADDR) {
        return s_arm7_arena;
    }
    if (addr == NDS_HEADER_COPY_ADDR_DS || addr == NDS_HEADER_COPY_ADDR_TWL) {
        return s_header_arena;
    }
    return s_arm9_arena;
}

void loader_host_test_handoff(const u32 *args)
{
    s_handoff_calls++;
    memcpy(s_handoff_args, args, sizeof(s_handoff_args));
}

/* ------------------------------------------------------------------ */
/* Image loading + sector-read callback                                */
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

/* ------------------------------------------------------------------ */
/* Pipeline                                                            */
/* ------------------------------------------------------------------ */

static void test_mbr_and_mount(void)
{
    fat_mbr_partition_t part;
    fat_volume_t vol;

    assert(fat_mbr_find_fat_partition(g_img, &part));
    assert(part.present);
    assert(part.type == FAT_PART_TYPE_FAT16 ||
           part.type == FAT_PART_TYPE_FAT16_LBA);
    assert(part.lba == 64u);
    assert(part.sector_count == g_img_sectors - 64u);

    assert(fat_mount(&vol, img_read, NULL));
    assert(vol.type == FAT_TYPE_FAT16);
    assert(vol.partition_lba == 64u);
    assert(vol.bytes_per_sector == FAT_SECTOR_SIZE);
}

static void test_root_listing(void)
{
    fat_volume_t vol;
    fat_dir_iter_t it;
    fat_file_entry_t e;
    bool hello = false;
    bool badcrc = false;
    bool overlap = false;
    bool readme_filtered = false;
    bool readme_seen = false;
    u32 listed = 0u;

    assert(fat_mount(&vol, img_read, NULL));

    /* .nds/.srl filter: HELLO/BADCRC/OVERLAP only. */
    fat_dir_iter_init(&vol, &it, true);
    while (fat_dir_next(&it, &e)) {
        listed++;
        if (strcmp(e.name, "HELLO.NDS") == 0) {
            hello = true;
        } else if (strcmp(e.name, "BADCRC.NDS") == 0) {
            badcrc = true;
        } else if (strcmp(e.name, "OVERLAP.NDS") == 0) {
            overlap = true;
        } else if (strcmp(e.name, "README.TXT") == 0) {
            readme_filtered = true;
        }
    }
    assert(hello && badcrc && overlap);
    assert(!readme_filtered);
    assert(listed == 3u);

    /* The unfiltered listing still sees README.TXT... */
    fat_dir_iter_init(&vol, &it, false);
    while (fat_dir_next(&it, &e)) {
        if (strcmp(e.name, "README.TXT") == 0) {
            readme_seen = true;
            assert(!fat_is_nds_entry(&e));
        }
    }
    /* ...and the browser filter would drop it. */
    assert(readme_seen);
}

/* Find a root entry by name (the iterator is the public API). */
static bool find_entry(fat_volume_t *vol, const char *name,
                       fat_file_entry_t *out)
{
    fat_dir_iter_t it;
    fat_file_entry_t e;

    fat_dir_iter_init(vol, &it, false);
    while (fat_dir_next(&it, &e)) {
        if (strcmp(e.name, name) == 0) {
            *out = e;
            return true;
        }
    }
    return false;
}

static void test_hello_validates_and_stages(void)
{
    fat_volume_t vol;
    fat_file_entry_t hello;
    nds_header_t hdr;
    loader_result_t res;
    u8 *full;
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

    assert(fat_mount(&vol, img_read, NULL));
    assert(find_entry(&vol, "HELLO.NDS", &hello));
    assert(hello.file_size > 0x4000u);   /* multi-cluster */

    /* 1. Validate (both the legacy DS and the extended TWL window). */
    assert(loader_validate_file(&vol, &hello, false, &hdr) == NDS_LOAD_OK);
    assert(loader_validate_file(&vol, &hello, true, &hdr) == NDS_LOAD_OK);
    assert(hdr.crc_ok);
    assert(strcmp(hdr.title, "CHAINLOAD OK") == 0);
    assert(strcmp(hdr.gamecode, "TSTL") == 0);
    assert(hdr.unit_code == NDS_UNIT_CODE_NDS_DSI);
    assert(hdr.arm9_ram_addr == 0x02004000u);
    assert(hdr.arm7_ram_addr == 0x02380000u);

    /* 2. Stage (dry run): both payloads copied, handoff requested once. */
    s_handoff_calls = 0u;
    memset(s_handoff_args, 0, sizeof(s_handoff_args));
    assert(loader_stage_and_boot(&vol, &hello, true, &res) == NDS_LOAD_OK);
    assert(res.status == NDS_LOAD_OK);
    assert(res.staged);
    assert(res.dry_run);
    assert(!res.booted);
    assert(res.arm9_entry == hdr.arm9_entry);
    assert(res.arm9_size == hdr.arm9_size);
    assert(res.arm7_entry == hdr.arm7_entry);
    assert(res.arm7_dst == hdr.arm7_ram_addr);
    assert(res.arm7_stage == LOADER_ARM7_BOUNCE_ADDR);
    assert(res.arm7_size == hdr.arm7_size);
    assert(s_handoff_calls == 1u);
    assert(s_handoff_args[0] == hdr.arm7_entry);
    assert(s_handoff_args[1] == LOADER_ARM7_BOUNCE_ADDR);
    assert(s_handoff_args[2] == hdr.arm7_ram_addr);
    assert(s_handoff_args[3] == hdr.arm7_size);

    /*
     * 3. The whole file streams back through the cluster-chain reader and the
     *    staged payloads match the bytes at their ROM offsets: this proves the
     *    multi-cluster/fragmented walk, not just the header read.
     */
    full = (u8 *)malloc(hello.file_size);
    assert(full != NULL);
    assert(fat_read_file_range(&vol, &hello, 0u, full, hello.file_size,
                               &got, &chain));
    assert(got == hello.file_size);
    assert(chain == FAT_CHAIN_OK);
    assert(memcmp(full + hdr.arm9_rom_offset, s_arm9_arena,
                  hdr.arm9_size) == 0);
    assert(memcmp(full + hdr.arm7_rom_offset, s_arm7_arena,
                  hdr.arm7_size) == 0);
    free(full);
}

static void test_negative_images(void)
{
    fat_volume_t vol;
    fat_file_entry_t badcrc;
    fat_file_entry_t overlap;
    nds_header_t hdr;
    loader_result_t res;

    assert(fat_mount(&vol, img_read, NULL));

    assert(find_entry(&vol, "BADCRC.NDS", &badcrc));
    assert(loader_validate_file(&vol, &badcrc, true, &hdr) == NDS_ERR_BAD_CRC);
    assert(loader_stage_and_boot(&vol, &badcrc, true, &res) == NDS_ERR_BAD_CRC);
    assert(!res.staged);
    assert(res.status == NDS_ERR_BAD_CRC);

    assert(find_entry(&vol, "OVERLAP.NDS", &overlap));
    assert(loader_validate_file(&vol, &overlap, true, &hdr)
           == NDS_ERR_OVERLAP_BOOTLOADER);
    assert(loader_stage_and_boot(&vol, &overlap, true, &res)
           == NDS_ERR_OVERLAP_BOOTLOADER);
    assert(!res.staged);
    assert(res.status == NDS_ERR_OVERLAP_BOOTLOADER);
}

/* ------------------------------------------------------------------ */

static int load_image(const char *path)
{
    FILE *fh;
    long size;

    fh = fopen(path, "rb");
    if (fh == NULL) {
        return -1;
    }
    if (fseek(fh, 0, SEEK_END) != 0) {
        fclose(fh);
        return -1;
    }
    size = ftell(fh);
    if (size <= 0 || (size % FAT_SECTOR_SIZE) != 0) {
        fclose(fh);
        return -1;
    }
    if (fseek(fh, 0, SEEK_SET) != 0) {
        fclose(fh);
        return -1;
    }
    g_img = (u8 *)malloc((size_t)size);
    if (g_img == NULL) {
        fclose(fh);
        return -1;
    }
    if (fread(g_img, 1u, (size_t)size, fh) != (size_t)size) {
        free(g_img);
        g_img = NULL;
        fclose(fh);
        return -1;
    }
    fclose(fh);
    g_img_sectors = (u32)(size / FAT_SECTOR_SIZE);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "build/sdcard.img";

    if (load_image(path) != 0) {
        fprintf(stderr,
                "test_sd_image: cannot read %s (run `make sd-image`)\n", path);
        return 1;
    }

    test_mbr_and_mount();
    test_root_listing();
    test_hello_validates_and_stages();
    test_negative_images();

    free(g_img);
    puts("test_sd_image: all assertions passed");
    return 0;
}
