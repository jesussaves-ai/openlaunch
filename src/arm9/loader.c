/*
 * src/arm9/loader.c
 *
 * Safe read-only .nds / .srl image loader, RAM stager and dual-CPU chainload
 * handoff. See include/arm9/loader.h.
 *
 * Safety properties (AGENTS.md §4/§5):
 *   - the header is fully validated (CRC + aligned, in-range ARM9/ARM7 load
 *     ranges that avoid the bootloader reservation) before any RAM is written;
 *   - the ARM7 payload is never staged directly over the running ARM7 loader:
 *     it is streamed through a shared bounce buffer and copied by the ARM7 at
 *     the last moment;
 *   - the actual CPU handoff is opt-in (ENABLE_CHAINLOAD_BOOT), so by default
 *     the pipeline is a dry run.
 */
#include "arm9/loader.h"
#include "arm9/dma.h"
#include "arm9/ipc.h"
#include "arm9/storage.h"
#include "arm9/video.h"
#include "common/cache.h"
#include "common/dsi_hw.h"
#include "common/ipc_protocol.h"
#include "common/mem.h"

/*
 * Header staging buffer. We read a full 2-sector (1024-byte) header when the
 * file allows it so the TWL ARM9i/ARM7i descriptors are available, but the
 * canonical 512-byte header copy is still taken from the first sector only.
 * Lives in .bss (shared Main RAM, inside the loader reservation) so it is
 * never confused with the destination image.
 */
#define LOADER_HEADER_READ_LEN 1024u
static u8 s_header_sector[LOADER_HEADER_READ_LEN];

/*
 * Bounded ARM9 prefix window for loader_detect_dldi() (the verify/dry-run
 * inspector). Lives in .bss (shared Main RAM) so the delegated ARM9 storage
 * reader can fill it directly over IPC. Never DTCM.
 */
static u8 s_dldi_scan_buf[LOADER_DLDI_SCAN_MAX];

/*
 * Banner read buffer (0x840 bytes) for loader_read_banner(). Lives in .bss
 * (shared Main RAM) so the delegated ARM9 storage reader can fill it directly
 * over IPC. Never DTCM.
 */
static u8 s_banner_buf[NDS_BANNER_SIZE];

#ifdef LOADER_HOST_TEST
/*
 * Host integration tests (tests/test_sd_image.c) run the full staging pipeline
 * against a synthetic FAT16 volume. They cannot write to the fixed ARM9/ARM7
 * load addresses and have no IPC peer, so when LOADER_HOST_TEST is defined the
 * stager redirects every absolute-address write into a scratch arena supplied
 * by the test and reports the handoff request instead of touching the IPC FIFO.
 *
 * The target build never defines LOADER_HOST_TEST, so the production path is
 * byte-for-byte unchanged.
 */
u8 *loader_host_test_stage(u32 addr, u32 len);
void loader_host_test_handoff(const u32 *args);
#endif

/* Destination of a payload write: identity on target, a test arena on host. */
static u8 *loader_stage_ptr(u32 addr, u32 len)
{
#ifdef LOADER_HOST_TEST
    return loader_host_test_stage(addr, len);
#else
    (void)len;
    return (u8 *)(uintptr_t)addr;
#endif
}

void loader_plan_from_header(const nds_header_t *hdr, bool extended_ram,
                             loader_plan_t *out)
{
    if (out == NULL) {
        return;
    }

    dsi_memset(out, 0, sizeof(*out));
    if (hdr == NULL) {
        return;
    }

    dsi_memcpy(&out->hdr, hdr, sizeof(out->hdr));
    out->twl_extended = extended_ram;

    /*
     * The ARM7 payload always goes through the shared bounce buffer: that is
     * the only way to reach ARM7-private WRAM, and it keeps a Main-RAM target
     * (e.g. 0x02380000, which aliases the running ARM7 loader) intact until
     * the very last moment.
     */
    out->arm7_bounce = true;
    out->arm7_stage_addr = LOADER_ARM7_BOUNCE_ADDR;
}

/* ---- Homebrew argv / __system_argv protocol --------------------------- */

u32 loader_format_command_line(char *cmdline, u32 cmdline_cap, const char *name)
{
    static const char prefix[4] = { 's', 'd', ':', '/' };
    u32 prefix_len = 4u;
    u32 name_len;
    u32 total;

    if (cmdline == NULL) {
        return 0u;
    }
    if (cmdline_cap > 0u) {
        cmdline[0] = '\0';
    }
    if (cmdline_cap == 0u || name == NULL || name[0] == '\0') {
        return 0u;
    }

    /* A leading slash is redundant: "sd:/" is always prepended. */
    while (*name == '/') {
        name++;
    }
    if (name[0] == '\0') {
        return 0u;
    }

    name_len = (u32)dsi_strlen(name);
    /* Need room for "sd:/" + name + NUL. */
    if (cmdline_cap <= prefix_len + 1u ||
        name_len > cmdline_cap - prefix_len - 1u) {
        return 0u;
    }

    total = prefix_len + name_len;
    dsi_memcpy(cmdline, prefix, prefix_len);
    dsi_memcpy(cmdline + prefix_len, name, name_len);
    cmdline[total] = '\0';
    return total;
}

u32 loader_build_argv(u8 *argv, char *cmdline, u32 cmdline_cap,
                      u32 cmdline_addr, const char *name)
{
    u32 len;

    if (argv == NULL) {
        return 0u;
    }

    len = loader_format_command_line(cmdline, cmdline_cap, name);
    if (len == 0u) {
        return 0u;
    }

    dsi_write_le32(argv + NDS_ARGV_MAGIC_OFF, NDS_ARGV_MAGIC);
    dsi_write_le32(argv + NDS_ARGV_CMD_OFF, cmdline_addr);
    dsi_write_le32(argv + NDS_ARGV_LEN_OFF, len);
    dsi_write_le32(argv + NDS_ARGV_ARGC_OFF, 1u);
    dsi_write_le32(argv + NDS_ARGV_ARG0_OFF, cmdline_addr);
    dsi_write_le32(argv + NDS_ARGV_END_OFF, NDS_ARGV_END_MARKER);
    return len;
}

u32 loader_stage_argv(const char *name, bool extended_ram)
{
    char cmdline[NDS_ARGV_CMDLINE_MAX];
    u8 argv[NDS_ARGV_HEADER_SIZE];
    u32 hdr_base = extended_ram ? NDS_HEADER_COPY_ADDR_TWL
                                : NDS_HEADER_COPY_ADDR_DS;
    u32 cmdline_addr = extended_ram ? NDS_ARGV_CMDLINE_ADDR_TWL
                                    : NDS_ARGV_CMDLINE_ADDR_DS;
    u32 len = loader_build_argv(argv, cmdline, sizeof(cmdline),
                                cmdline_addr, name);

    if (len == 0u) {
        return 0u;
    }

    dsi_memcpy(loader_stage_ptr(cmdline_addr, len + 1u), cmdline, len + 1u);
    dsi_memcpy(loader_stage_ptr(hdr_base + NDS_ARGV_OFFSET,
                                NDS_ARGV_HEADER_SIZE),
               argv, NDS_ARGV_HEADER_SIZE);
    return len;
}

nds_load_status_t loader_validate_file(fat_volume_t *vol,
                                       const fat_file_entry_t *entry,
                                       bool extended_ram,
                                       nds_header_t *out_hdr)
{
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;
    nds_load_bounds_t bounds;
    nds_load_status_t status;

    if (out_hdr != NULL) {
        dsi_memset(out_hdr, 0, sizeof(*out_hdr));
    }
    if (vol == NULL || entry == NULL) {
        return NDS_ERR_FILE_TRUNCATED;
    }
    if (entry->file_size < NDS_HEADER_SECTOR_SIZE) {
        return NDS_ERR_FILE_TRUNCATED;
    }

    /*
     * Read a bounded 2-sector header (clamped to the file): the base fields
     * live in sector 0, the TWL ARM9i/ARM7i descriptors immediately after.
     */
    if (!fat_read_file_range(vol, entry, 0u, s_header_sector,
                             LOADER_HEADER_READ_LEN, &got, &chain) ||
        got < NDS_HEADER_SECTOR_SIZE) {
        return NDS_ERR_FILE_TRUNCATED;
    }

    nds_load_bounds_default(&bounds, extended_ram);
    status = nds_header_validate_loadable(s_header_sector, got,
                                          entry->file_size, &bounds, out_hdr);
    if (status != NDS_LOAD_OK) {
        return status;
    }

    /*
     * `out_hdr` now also carries `banner_offset` (0x068), so the [6]
     * verify/dry-run inspector can fetch and decode the banner block with
     * loader_read_banner(). A missing or malformed banner never affects the
     * loadability verdict.
     */

    /*
     * In TWL mode, a DSi-capable image may carry optional ARM9i/ARM7i segments.
     * They are validated even when absent (a plain hybrid image has none).
     */
    if (extended_ram) {
        nds_header_t tmp;
        nds_header_t *h = (out_hdr != NULL) ? out_hdr : &tmp;

        return nds_header_validate_twl_segments(h, entry->file_size, &bounds);
    }

    return status;
}

bool loader_detect_dldi(fat_volume_t *vol, const fat_file_entry_t *entry,
                        const nds_header_t *hdr, dldi_info_t *out)
{
    u32 want;
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
    }
    if (vol == NULL || entry == NULL || hdr == NULL || hdr->arm9_size == 0u) {
        return false;
    }

    want = hdr->arm9_size;
    if (want > LOADER_DLDI_SCAN_MAX) {
        want = LOADER_DLDI_SCAN_MAX;
    }

    if (!fat_read_file_range(vol, entry, hdr->arm9_rom_offset,
                             s_dldi_scan_buf, want, &got, &chain)) {
        return false;
    }
    if (got < DLDI_HEADER_SIZE) {
        return false;
    }
    return dldi_scan(s_dldi_scan_buf, got, out, NULL);
}

bool loader_read_banner(fat_volume_t *vol, const fat_file_entry_t *entry,
                        const nds_header_t *hdr, nds_banner_info_t *out)
{
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
    }
    if (vol == NULL || entry == NULL || hdr == NULL || out == NULL) {
        return false;
    }
    if (hdr->banner_offset == 0u) {
        return false;
    }
    /* The whole 0x840-byte block must lie inside the file. */
    if (hdr->banner_offset > entry->file_size ||
        entry->file_size - hdr->banner_offset < NDS_BANNER_SIZE) {
        return false;
    }
    if (!fat_read_file_range(vol, entry, hdr->banner_offset,
                             s_banner_buf, NDS_BANNER_SIZE, &got, &chain) ||
        got != NDS_BANNER_SIZE) {
        return false;
    }
    return nds_banner_parse(s_banner_buf, NDS_BANNER_SIZE, out);
}

#if defined(ENABLE_CHAINLOAD_BOOT) && \
    (defined(__arm__) || defined(__thumb__))

/*
 * Final ARM9 shutdown and branch. Compiled only when the boot guard is armed.
 *
 * Disables all interrupt sources, stops the four hardware timers, resets the
 * ARM9 DMA engines and the 2D display, flushes the caches and branches to the
 * staged entry point. Never returns.
 */
static void loader_jump_arm9(u32 entry) __attribute__((noreturn));

static void loader_jump_arm9(u32 entry)
{
    u32 i;

    REG16(IRQ_IME) = 0u;
    REG32(IRQ_IE) = 0u;
    REG32(IRQ_IE2) = 0u;
    REG32(IRQ_IF) = 0xFFFFFFFFu;
    REG32(IRQ_IF2) = 0xFFFFFFFFu;

    for (i = 0u; i < 4u; i++) {
        REG16(TIMER0_CNT + i * 4u) = 0u;
    }

    arm9_dma_reset();
    arm9_dma_aux_reset();
    arm9_video_reset();

    cache_flush_all();

    ((void (*)(void))(uintptr_t)entry)();

    for (;;) {
    }
}
#endif /* ENABLE_CHAINLOAD_BOOT */

nds_load_status_t loader_stage_and_boot_path(fat_volume_t *vol,
                                             const fat_file_entry_t *entry,
                                             const char *path,
                                             bool extended_ram,
                                             loader_result_t *out)
{
    nds_header_t hdr;
    nds_load_status_t status;
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;
    const char *launch_name = (path != NULL && path[0] != '\0')
                                  ? path : entry ? entry->name : NULL;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
    }

    status = loader_validate_file(vol, entry, extended_ram, &hdr);
    if (out != NULL) {
        out->status = status;
        out->arm9_entry = hdr.arm9_entry;
        out->arm9_size = hdr.arm9_size;
        out->arm7_entry = hdr.arm7_entry;
        out->arm7_dst = hdr.arm7_ram_addr;
        out->arm7_size = hdr.arm7_size;
        out->arm7_stage = LOADER_ARM7_BOUNCE_ADDR;
        out->arm9i_size = hdr.arm9i_size;
        out->arm7i_size = hdr.arm7i_size;
        out->dry_run = true;
    }
    if (status != NDS_LOAD_OK) {
        return status;
    }

    /* The ARM7 payload must fit the shared bounce buffer. */
    if (hdr.arm7_size > LOADER_ARM7_BOUNCE_MAX) {
        if (out != NULL) {
            out->status = NDS_ERR_ARM7_BOUNDS;
        }
        return NDS_ERR_ARM7_BOUNDS;
    }

    /*
     * 1. Stream the ARM9 payload straight into its validated load address.
     *    It cannot overlap the loader reservation, so this is safe even while
     *    this function is executing.
     */
    if (!fat_read_file_range(vol, entry, hdr.arm9_rom_offset,
                             loader_stage_ptr(hdr.arm9_ram_addr, hdr.arm9_size),
                             hdr.arm9_size, &got, &chain) ||
        got != hdr.arm9_size) {
        if (out != NULL) {
            out->status = NDS_ERR_FILE_TRUNCATED;
        }
        return NDS_ERR_FILE_TRUNCATED;
    }

    /*
     * 1b. DLDI detection / in-RAM patch. The ARM9 segment is now staged, so a
     *     DLDI stub can be found and (when it is the default "NONE" placeholder
     *     and space permits) replaced with the built-in clean-room read-only
     *     driver before any cache flush or handoff. This only rewrites the
     *     staged RAM image; no hardware register is touched.
     */
    if (out != NULL) {
        out->dldi_present = false;
        out->dldi_patched = false;
        out->dldi_alloc_kb = 0u;
        out->dldi_io_type[0] = '\0';
    }
    {
        u8 *staged = loader_stage_ptr(hdr.arm9_ram_addr, hdr.arm9_size);
        dldi_info_t dinfo;

        if (dldi_scan(staged, hdr.arm9_size, &dinfo, NULL)) {
            if (out != NULL) {
                out->dldi_present = true;
                out->dldi_alloc_kb = dinfo.allocated_size >> 10;
                dsi_memcpy(out->dldi_io_type, dinfo.io_type,
                           sizeof(out->dldi_io_type));
            }

            if (dsi_memcmp(dinfo.io_type, "NONE", 5u) == 0) {
                const u8 *ro = dldi_builtin_ro_driver();

                if (dldi_apply_patch(staged, hdr.arm9_size, hdr.arm9_ram_addr,
                                     ro, DLDI_BUILTIN_RO_SIZE)) {
                    if (out != NULL) {
                        out->dldi_patched = true;
                        dsi_memcpy(out->dldi_io_type,
                                   DLDI_BUILTIN_RO_IO_TYPE,
                                   sizeof(out->dldi_io_type));
                    }
                }
            }
        }
    }

    /*
     * 2. Stage the ARM7 payload into shared RAM; the ARM7 copies it to its
     *    final address (Main RAM or ARM7 WRAM) as the last step of the
     *    handoff, avoiding a self-overwrite.
     */
    if (!fat_read_file_range(vol, entry, hdr.arm7_rom_offset,
                             loader_stage_ptr(LOADER_ARM7_BOUNCE_ADDR,
                                              hdr.arm7_size),
                             hdr.arm7_size, &got, &chain) ||
        got != hdr.arm7_size) {
        if (out != NULL) {
            out->status = NDS_ERR_FILE_TRUNCATED;
        }
        return NDS_ERR_FILE_TRUNCATED;
    }

    /*
     * 2b. TWL extended segments. A DSi-capable image may carry optional ARM9i
     *     and ARM7i binaries alongside the primaries; in TWL mode stream each
     *     non-zero segment straight to its validated RAM address (Main RAM or
     *     NWRAM, both directly writable from the ARM9).
     */
    if (extended_ram && hdr.twl_capable && hdr.arm9i_size != 0u) {
        if (!fat_read_file_range(vol, entry, hdr.arm9i_rom_offset,
                                 loader_stage_ptr(hdr.arm9i_ram_address,
                                                  hdr.arm9i_size),
                                 hdr.arm9i_size, &got, &chain) ||
            got != hdr.arm9i_size) {
            if (out != NULL) {
                out->status = NDS_ERR_FILE_TRUNCATED;
            }
            return NDS_ERR_FILE_TRUNCATED;
        }
    }
    if (extended_ram && hdr.twl_capable && hdr.arm7i_size != 0u) {
        if (!fat_read_file_range(vol, entry, hdr.arm7i_rom_offset,
                                 loader_stage_ptr(hdr.arm7i_ram_address,
                                                  hdr.arm7i_size),
                                 hdr.arm7i_size, &got, &chain) ||
            got != hdr.arm7i_size) {
            if (out != NULL) {
                out->status = NDS_ERR_FILE_TRUNCATED;
            }
            return NDS_ERR_FILE_TRUNCATED;
        }
    }

    /*
     * 3. Publish the 512-byte header copy at the canonical firmware address
     *    (0x027FFE00 on DS / 0x02FFFE00 on a 16 MiB DSi), then make every
     *    staged buffer visible to the ARM7 and the fresh instruction stream.
     */
    dsi_memcpy(loader_stage_ptr(extended_ram ? NDS_HEADER_COPY_ADDR_TWL
                                             : NDS_HEADER_COPY_ADDR_DS,
                                NDS_HEADER_SECTOR_SIZE),
               s_header_sector, NDS_HEADER_SECTOR_SIZE);

    /*
     * 3b. Populate the canonical __system_argv structure so the launched
     *     homebrew learns its launch path ("sd:/<NAME>").
     */
    if (out != NULL) {
        out->argv_len = loader_stage_argv(launch_name, extended_ram);
    } else {
        (void)loader_stage_argv(launch_name, extended_ram);
    }

    dc_flush_range((const void *)(uintptr_t)hdr.arm9_ram_addr, hdr.arm9_size);
    dc_flush_range((const void *)(uintptr_t)LOADER_ARM7_BOUNCE_ADDR,
                   hdr.arm7_size);
    cache_flush_all();

    /*
     * 4. Ask the ARM7 to take the handoff. It copies the staged ARM7 payload
     *    to `arm7_ram_addr` and, when the boot guard is armed, branches to
     *    `arm7_entry`.
     */
    {
        u32 args[IPC_REQ_BOOT_HANDOFF_WORDS];
        u32 reply[IPC_REPLY_BOOT_WORDS + 1u];

        args[0] = hdr.arm7_entry;
        args[1] = LOADER_ARM7_BOUNCE_ADDR;
        args[2] = hdr.arm7_ram_addr;
        args[3] = hdr.arm7_size;

#ifdef LOADER_HOST_TEST
        (void)reply;
        loader_host_test_handoff(args);
#else
        (void)arm9_ipc_query_args(IPC_CH_SYSTEM, IPC_CMD_BOOT_HANDOFF,
                                  args, IPC_REQ_BOOT_HANDOFF_WORDS,
                                  reply, ARRAY_SIZE(reply),
                                  ARM9_STORAGE_IPC_TIMEOUT);
#endif
    }

    if (out != NULL) {
        out->staged = true;
        out->status = NDS_LOAD_OK;
    }

#if defined(ENABLE_CHAINLOAD_BOOT) && \
    (defined(__arm__) || defined(__thumb__))
    loader_jump_arm9(hdr.arm9_entry);
    if (out != NULL) {
        out->booted = true;
        out->dry_run = false;
    }
#endif

    if (out != NULL) {
        out->dry_run = true;
    }
    return NDS_LOAD_OK;
}

nds_load_status_t loader_stage_and_boot(fat_volume_t *vol,
                                        const fat_file_entry_t *entry,
                                        bool extended_ram,
                                        loader_result_t *out)
{
    return loader_stage_and_boot_path(vol, entry, NULL, extended_ram, out);
}
