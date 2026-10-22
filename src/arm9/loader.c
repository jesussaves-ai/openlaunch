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

/* Sector-0 staging buffer. Lives in .bss (shared Main RAM, inside the loader
 * reservation) so it is never confused with the destination image. */
static u8 s_header_sector[NDS_HEADER_SECTOR_SIZE];

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

    out->hdr = *hdr;
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

nds_load_status_t loader_validate_file(fat_volume_t *vol,
                                       const fat_file_entry_t *entry,
                                       bool extended_ram,
                                       nds_header_t *out_hdr)
{
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;
    nds_load_bounds_t bounds;

    if (out_hdr != NULL) {
        dsi_memset(out_hdr, 0, sizeof(*out_hdr));
    }
    if (vol == NULL || entry == NULL) {
        return NDS_ERR_FILE_TRUNCATED;
    }
    if (entry->file_size < NDS_HEADER_SECTOR_SIZE) {
        return NDS_ERR_FILE_TRUNCATED;
    }

    /* Read exactly sector 0 (the first cluster always contains it). */
    if (!fat_read_file_range(vol, entry, 0u, s_header_sector,
                             NDS_HEADER_SECTOR_SIZE, &got, &chain) ||
        got != NDS_HEADER_SECTOR_SIZE) {
        return NDS_ERR_FILE_TRUNCATED;
    }

    nds_load_bounds_default(&bounds, extended_ram);
    return nds_header_validate_loadable(s_header_sector, got,
                                        entry->file_size, &bounds, out_hdr);
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

nds_load_status_t loader_stage_and_boot(fat_volume_t *vol,
                                        const fat_file_entry_t *entry,
                                        bool extended_ram,
                                        loader_result_t *out)
{
    nds_header_t hdr;
    nds_load_status_t status;
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

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
                             (u8 *)(uintptr_t)hdr.arm9_ram_addr,
                             hdr.arm9_size, &got, &chain) ||
        got != hdr.arm9_size) {
        if (out != NULL) {
            out->status = NDS_ERR_FILE_TRUNCATED;
        }
        return NDS_ERR_FILE_TRUNCATED;
    }

    /*
     * 2. Stage the ARM7 payload into shared RAM; the ARM7 copies it to its
     *    final address (Main RAM or ARM7 WRAM) as the last step of the
     *    handoff, avoiding a self-overwrite.
     */
    if (!fat_read_file_range(vol, entry, hdr.arm7_rom_offset,
                             (u8 *)(uintptr_t)LOADER_ARM7_BOUNCE_ADDR,
                             hdr.arm7_size, &got, &chain) ||
        got != hdr.arm7_size) {
        if (out != NULL) {
            out->status = NDS_ERR_FILE_TRUNCATED;
        }
        return NDS_ERR_FILE_TRUNCATED;
    }

    /*
     * 3. Publish the 512-byte header copy at the canonical firmware address
     *    (0x027FFE00 on DS / 0x02FFFE00 on a 16 MiB DSi), then make every
     *    staged buffer visible to the ARM7 and the fresh instruction stream.
     */
    dsi_memcpy((void *)(uintptr_t)(extended_ram ? NDS_HEADER_COPY_ADDR_TWL
                                                : NDS_HEADER_COPY_ADDR_DS),
               s_header_sector, NDS_HEADER_SECTOR_SIZE);

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

        (void)arm9_ipc_query_args(IPC_CH_SYSTEM, IPC_CMD_BOOT_HANDOFF,
                                  args, IPC_REQ_BOOT_HANDOFF_WORDS,
                                  reply, ARRAY_SIZE(reply),
                                  ARM9_STORAGE_IPC_TIMEOUT);
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
