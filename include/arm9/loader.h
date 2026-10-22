/*
 * include/arm9/loader.h
 *
 * Safe read-only .nds / .srl image loader, RAM stager and dual-CPU chainload
 * handoff for the ARM9.
 *
 * Pipeline:
 *   1. loader_validate_file()  read sector 0 and run nds_header_validate_loadable()
 *   2. loader_stage_and_boot() stream the ARM9 payload into its load address,
 *      stage the ARM7 payload into the shared bounce buffer, copy the header
 *      to the canonical firmware address and trigger IPC_CMD_BOOT_HANDOFF.
 *
 * The actual CPU handoff (branch into the staged image) is compiled in only
 * when ENABLE_CHAINLOAD_BOOT is defined, mirroring the AGENTS.md §5
 * sandbox-first / default-off policy. Without it the whole staging and IPC
 * preparation runs and the function returns with result->dry_run set, so host
 * tests and diagnostics can exercise the pipeline without jumping into
 * arbitrary RAM.
 */
#ifndef DSI_ARM9_LOADER_H
#define DSI_ARM9_LOADER_H

#include "../common/types.h"
#include "../common/fat_ro.h"
#include "../common/nds_header.h"

/*
 * Shared-RAM bounce buffer used to stage the ARM7 segment before the handoff.
 * It sits just below the linked ARM9 image (0x02200000) and above the canonical
 * low-Main-RAM load addresses, so a new image can target either 0x02000000 or
 * the ARM7-private WRAM without clobbering the running loader.
 */
#define LOADER_ARM7_BOUNCE_ADDR 0x02100000u
#define LOADER_ARM7_BOUNCE_MAX  0x00100000u   /* 1 MiB */

/* Plan derived from a validated header (pure; host-testable). */
typedef struct {
    nds_header_t hdr;
    bool twl_extended;     /* 16 MiB Main-RAM window selected             */
    bool arm7_bounce;      /* ARM7 staged through the shared bounce buffer */
    u32  arm7_stage_addr;  /* where the ARM7 payload is staged             */
} loader_plan_t;

/* Fill `out` from an already-validated header. Pure. */
void loader_plan_from_header(const nds_header_t *hdr, bool extended_ram,
                             loader_plan_t *out);

/* Outcome of a staging / handoff attempt. */
typedef struct {
    nds_load_status_t status;
    bool staged;           /* both payloads copied to RAM (or bounce)     */
    bool booted;           /* the CPU handoff was actually performed      */
    bool dry_run;          /* built without ENABLE_CHAINLOAD_BOOT         */
    u32  arm9_entry;
    u32  arm9_size;
    u32  arm7_entry;
    u32  arm7_stage;       /* shared-RAM source handed to the ARM7        */
    u32  arm7_dst;         /* final ARM7 RAM destination                  */
    u32  arm7_size;
} loader_result_t;

/* Read sector 0 of `entry` through `vol` and validate it. */
nds_load_status_t loader_validate_file(fat_volume_t *vol,
                                       const fat_file_entry_t *entry,
                                       bool extended_ram,
                                       nds_header_t *out_hdr);

/*
 * Stage and hand off. Returns a specific nds_load_status_t. With
 * ENABLE_CHAINLOAD_BOOT defined and NDS_LOAD_OK this does not return (the ARM9
 * jumps into the new image); otherwise it performs all staging/IPC work and
 * returns with `out->dry_run` set.
 */
nds_load_status_t loader_stage_and_boot(fat_volume_t *vol,
                                        const fat_file_entry_t *entry,
                                        bool extended_ram,
                                        loader_result_t *out);

#endif /* DSI_ARM9_LOADER_H */
