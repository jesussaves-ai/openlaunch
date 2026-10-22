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
#include "arm9/boot_cfg.h"
#include "arm9/loader.h"

/* ------------------------------------------------------------------ */
/* LOADER_HOST_TEST staging hooks (see src/arm9/loader.c)              */
/* ------------------------------------------------------------------ */

#define ARENA_BYTES (1u << 20)   /* 1 MiB, matches LOADER_ARM7_BOUNCE_MAX */

static u8  s_arm9_arena[ARENA_BYTES];
static u8  s_arm7_arena[ARENA_BYTES];
static u8  s_header_arena[FAT_SECTOR_SIZE];
static u8  s_cmdline_arena[NDS_ARGV_CMDLINE_MAX];
static u32 s_handoff_calls;
static u32 s_handoff_args[4];

/* True when [addr, addr+len) lies inside the 512-byte header-copy page. */
static bool in_header_page(u32 addr, u32 base)
{
    return addr >= base && (addr - base) < NDS_HEADER_SECTOR_SIZE;
}

u8 *loader_host_test_stage(u32 addr, u32 len)
{
    (void)len;

    if (addr == LOADER_ARM7_BOUNCE_ADDR) {
        return s_arm7_arena;
    }
    if (in_header_page(addr, NDS_HEADER_COPY_ADDR_DS)) {
        return s_header_arena + (addr - NDS_HEADER_COPY_ADDR_DS);
    }
    if (in_header_page(addr, NDS_HEADER_COPY_ADDR_TWL)) {
        return s_header_arena + (addr - NDS_HEADER_COPY_ADDR_TWL);
    }
    if (addr == NDS_ARGV_CMDLINE_ADDR_DS || addr == NDS_ARGV_CMDLINE_ADDR_TWL) {
        return s_cmdline_arena;
    }
    return s_arm9_arena;
}

void loader_host_test_handoff(const u32 *args)
{
    s_handoff_calls++;
    memcpy(s_handoff_args, args, sizeof(s_handoff_args));
}

static u32 le32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) |
           ((u32)p[2] << 16) | ((u32)p[3] << 24);
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
    bool boot = false;
    bool badcrc = false;
    bool overlap = false;
    bool readme_filtered = false;
    bool readme_seen = false;
    u32 listed = 0u;

    assert(fat_mount(&vol, img_read, NULL));

    /* .nds/.srl filter: HELLO/BOOT/BADCRC/OVERLAP only. */
    fat_dir_iter_init(&vol, &it, true);
    while (fat_dir_next(&it, &e)) {
        listed++;
        if (strcmp(e.name, "HELLO.NDS") == 0) {
            hello = true;
        } else if (strcmp(e.name, "BOOT.NDS") == 0) {
            boot = true;
        } else if (strcmp(e.name, "BADCRC.NDS") == 0) {
            badcrc = true;
        } else if (strcmp(e.name, "OVERLAP.NDS") == 0) {
            overlap = true;
        } else if (strcmp(e.name, "README.TXT") == 0) {
            readme_filtered = true;
        }
    }
    assert(hello && boot && badcrc && overlap);
    assert(!readme_filtered);
    assert(listed == 4u);

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

/*
 * Validate `entry`, read its banner through the delegated FAT reader and assert
 * the decoded title. Also confirms the banner sits at the canonical 0x840.
 */
static void assert_banner(fat_volume_t *vol, const fat_file_entry_t *entry,
                          const char *title)
{
    nds_header_t hdr;
    nds_banner_info_t b;

    assert(loader_validate_file(vol, entry, true, &hdr) == NDS_LOAD_OK);
    assert(hdr.banner_offset == 0x840u);
    assert(loader_read_banner(vol, entry, &hdr, &b));
    assert(b.version_ok);
    assert(b.crc_ok);
    assert(strcmp(b.title, title) == 0);
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

    /* 1b. The banner referenced by 0x068 decodes cleanly. */
    assert_banner(&vol, &hello, "CHAINLOAD OK");

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
     * 2a. The ARM9 segment carried the default "NONE" DLDI stub from offset 0
     *     and the staging path patched it in RAM with the built-in read-only
     *     driver before the handoff.
     */
    assert(res.dldi_present);
    assert(res.dldi_patched);
    assert(res.dldi_alloc_kb == 1u);
    assert(strcmp(res.dldi_io_type, "DSiR") == 0);
    {
        dldi_info_t dinfo;

        assert(dldi_parse_header(s_arm9_arena, hdr.arm9_size, &dinfo));
        assert(strcmp(dinfo.io_type, "DSiR") == 0);
        assert(dinfo.allocated_size == 1024u);
    }

    /*
     * 2b. The canonical __system_argv was published: the command line at
     *     NDS_ARGV_CMDLINE_ADDR_TWL and the 24-byte header inside the copied
     *     header page at header_base + 0x70.
     */
    assert(res.argv_len == 13u);
    assert(strcmp((char *)s_cmdline_arena, "sd:/HELLO.NDS") == 0);
    {
        const u8 *av = s_header_arena + NDS_ARGV_OFFSET;

        assert(le32(av + NDS_ARGV_MAGIC_OFF) == NDS_ARGV_MAGIC);
        assert(le32(av + NDS_ARGV_CMD_OFF) == NDS_ARGV_CMDLINE_ADDR_TWL);
        assert(le32(av + NDS_ARGV_LEN_OFF) == 13u);
        assert(le32(av + NDS_ARGV_ARGC_OFF) == 1u);
        assert(le32(av + NDS_ARGV_ARG0_OFF) == NDS_ARGV_CMDLINE_ADDR_TWL);
        assert(le32(av + NDS_ARGV_END_OFF) == NDS_ARGV_END_MARKER);
    }

    /*
     * 3. The whole file streams back through the cluster-chain reader. The
     *    staged ARM9 segment was patched in place at offset 0 (the DLDI stub),
     *    so only the region after the replaced driver must still match the
     *    on-disk bytes; the ARM7 segment must match exactly. This proves the
     *    multi-cluster/fragmented walk, not just the header read.
     */
    full = (u8 *)malloc(hello.file_size);
    assert(full != NULL);
    assert(fat_read_file_range(&vol, &hello, 0u, full, hello.file_size,
                               &got, &chain));
    assert(got == hello.file_size);
    assert(chain == FAT_CHAIN_OK);
    assert(hdr.arm9_size > DLDI_BUILTIN_RO_SIZE);
    assert(memcmp(full + hdr.arm9_rom_offset + DLDI_BUILTIN_RO_SIZE,
                  s_arm9_arena + DLDI_BUILTIN_RO_SIZE,
                  hdr.arm9_size - DLDI_BUILTIN_RO_SIZE) == 0);
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

    /* The synthetic stub carries no banner (banner_offset 0). */
    assert(hdr.banner_offset == 0u);
    {
        nds_banner_info_t binfo;

        assert(!loader_read_banner(&vol, &badcrc, &hdr, &binfo));
    }

    assert(find_entry(&vol, "OVERLAP.NDS", &overlap));
    assert(loader_validate_file(&vol, &overlap, true, &hdr)
           == NDS_ERR_OVERLAP_BOOTLOADER);
    assert(loader_stage_and_boot(&vol, &overlap, true, &res)
           == NDS_ERR_OVERLAP_BOOTLOADER);
    assert(!res.staged);
    assert(res.status == NDS_ERR_OVERLAP_BOOTLOADER);
}

/*
 * Unlaunch-style autoboot target: BOOT.NDS must be discoverable on the SD root,
 * validate cleanly and stage through the same pipeline, publishing the
 * "sd:/BOOT.NDS" command line.
 */
static void test_boot_nds(void)
{
    fat_volume_t vol;
    fat_file_entry_t boot;
    nds_header_t hdr;
    loader_result_t res;

    assert(fat_mount(&vol, img_read, NULL));
    assert(find_entry(&vol, "BOOT.NDS", &boot));

    assert(loader_validate_file(&vol, &boot, true, &hdr) == NDS_LOAD_OK);
    assert(hdr.crc_ok);
    assert(strcmp(hdr.title, "AUTOBOOT OK") == 0);
    assert(strcmp(hdr.gamecode, "TSTL") == 0);
    assert(hdr.unit_code == NDS_UNIT_CODE_NDS_DSI);

    /* The banner was retitled in step with the ROM title. */
    assert_banner(&vol, &boot, "AUTOBOOT OK");

    s_handoff_calls = 0u;
    assert(loader_stage_and_boot(&vol, &boot, true, &res) == NDS_LOAD_OK);
    assert(res.staged);
    assert(res.dry_run);
    assert(s_handoff_calls == 1u);
    assert(res.argv_len == 12u);   /* "sd:/BOOT.NDS" */
    assert(strcmp((char *)s_cmdline_arena, "sd:/BOOT.NDS") == 0);
    assert(res.dldi_present);
    assert(res.dldi_patched);
    assert(strcmp(res.dldi_io_type, "DSiR") == 0);
}

/* ------------------------------------------------------------------ */

/*
 * One-level subdirectory + BOOT.CFG discovery: NDS/SUBAPP.NDS resolves through
 * fat_find_path() and the root BOOT.CFG is readable and maps DEFAULT/A/X.
 */
static void test_subdir_and_bootcfg(void)
{
    fat_volume_t vol;
    fat_file_entry_t e;
    u8 buf[BOOT_CFG_READ_LEN];
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

    assert(fat_mount(&vol, img_read, NULL));

    /* Root file, with and without a leading slash. */
    assert(fat_find_path(&vol, "BOOT.NDS", &e));
    assert(!e.is_dir);
    assert(fat_find_path(&vol, "/HELLO.NDS", &e));

    /* The NDS directory and its subdirectory file. */
    assert(fat_find_path(&vol, "NDS", &e));
    assert(e.is_dir);
    assert(fat_find_path(&vol, "NDS/SUBAPP.NDS", &e));
    assert(!e.is_dir && e.file_size > 0x4000u);
    assert(fat_find_path(&vol, "/NDS/SUBAPP.NDS", &e));

    /* Negative cases. */
    assert(!fat_find_path(&vol, "NOPE/SUBAPP.NDS", &e));
    assert(!fat_find_path(&vol, "NDS/MISSING.NDS", &e));
    assert(!fat_find_path(&vol, "HELLO.NDS/SUBAPP.NDS", &e));
    assert(!fat_find_path(&vol, "A/B/C.NDS", &e));

    /* BOOT.CFG is a root file and parses into the expected map. */
    {
        boot_cfg_t cfg;
        fat_file_entry_t cfg_entry;

        assert(fat_find_path(&vol, "BOOT.CFG", &cfg_entry));
        assert(!cfg_entry.is_dir);
        assert(fat_read_file_range(&vol, &cfg_entry, 0u, buf, sizeof(buf),
                                   &got, &chain));
        assert(got > 0u && got < sizeof(buf));
        assert(boot_cfg_parse(buf, got, &cfg));
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_X], "NDS/SUBAPP.NDS") == 0);
    }
}

/*
 * End-to-end staging of NDS/SUBAPP.NDS: validate, stage and confirm the
 * published __system_argv carries "sd:/NDS/SUBAPP.NDS".
 */
static void test_subapp_stages(void)
{
    fat_volume_t vol;
    fat_file_entry_t sub;
    nds_header_t hdr;
    loader_result_t res;
    const char *path = "NDS/SUBAPP.NDS";

    assert(fat_mount(&vol, img_read, NULL));
    assert(fat_find_path(&vol, path, &sub));
    assert(!sub.is_dir);

    assert(loader_validate_file(&vol, &sub, true, &hdr) == NDS_LOAD_OK);
    assert(hdr.crc_ok);
    assert(strncmp(hdr.title, "SUBDIR", 6u) == 0);
    assert(strcmp(hdr.gamecode, "TSTL") == 0);
    assert(hdr.unit_code == NDS_UNIT_CODE_NDS_DSI);

    /* The /NDS payload's banner carries the full (untruncated) title. */
    assert_banner(&vol, &sub, "SUBDIR PAYLOAD");

    s_handoff_calls = 0u;
    assert(loader_stage_and_boot_path(&vol, &sub, path, true, &res)
           == NDS_LOAD_OK);
    assert(res.staged);
    assert(res.dry_run);
    assert(s_handoff_calls == 1u);
    assert(res.argv_len == 18u);   /* "sd:/NDS/SUBAPP.NDS" */
    assert(strcmp((char *)s_cmdline_arena, "sd:/NDS/SUBAPP.NDS") == 0);
    assert(res.dldi_present);
    assert(res.dldi_patched);
    assert(strcmp(res.dldi_io_type, "DSiR") == 0);
    {
        const u8 *av = s_header_arena + NDS_ARGV_OFFSET;

        assert(le32(av + NDS_ARGV_MAGIC_OFF) == NDS_ARGV_MAGIC);
        assert(le32(av + NDS_ARGV_LEN_OFF) == 18u);
    }
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
    test_boot_nds();
    test_subdir_and_bootcfg();
    test_subapp_stages();

    free(g_img);
    puts("test_sd_image: all assertions passed");
    return 0;
}
