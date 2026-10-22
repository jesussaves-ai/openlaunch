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
    u32  arm9i_size;       /* TWL ARM9i streamed (0 when absent)          */
    u32  arm7i_size;       /* TWL ARM7i streamed (0 when absent)          */
    u32  argv_len;         /* bytes in the populated command line         */
} loader_result_t;

/* Read sector 0 of `entry` through `vol` and validate it. */
nds_load_status_t loader_validate_file(fat_volume_t *vol,
                                       const fat_file_entry_t *entry,
                                       bool extended_ram,
                                       nds_header_t *out_hdr);

/* ---- Homebrew argv / __system_argv protocol (pure) -------------------- */

/*
 * Build the canonical command line "sd:/<name>" (NUL-terminated) into
 * `cmdline` (capacity `cmdline_cap`).
 *
 * `name` may be a root 8.3 name ("BOOT.NDS") or a one-level path
 * ("NDS/HELLO.NDS"); a leading slash is ignored so "/BOOT.NDS" and "BOOT.NDS"
 * format identically. Returns the command-line length excluding the NUL, or 0
 * when the inputs are invalid (NULL destination, empty name) or the path does
 * not fit `cmdline_cap` including its terminator. `cmdline` is left
 * NUL-terminated when capacity permits even on failure.
 */
u32 loader_format_command_line(char *cmdline, u32 cmdline_cap, const char *name);

/*
 * Build the 24-byte __system_argv header into `argv` and the command line into
 * `cmdline`, following the devkitPro/BlocksDS launch contract.
 *
 *   argv        : NDS_ARGV_HEADER_SIZE-byte destination for the header
 *   cmdline     : NUL-terminated command-line destination
 *   cmdline_cap : capacity of `cmdline`
 *   cmdline_addr: RAM address of `cmdline` (stored in the header)
 *   name        : 8.3 file name, e.g. "HELLO.NDS"
 *
 * Writes argvMagic = NDS_ARGV_MAGIC, the command-line pointer/length,
 * argc = 1, argv[0] = cmdline_addr and the 0xFFFFFFFF terminator.
 *
 * Returns the command-line length (excluding the NUL), or 0 on failure. Never
 * reads or writes outside the supplied buffers.
 */
u32 loader_build_argv(u8 *argv, char *cmdline, u32 cmdline_cap,
                      u32 cmdline_addr, const char *name);

/*
 * Populate the real high-RAM argv region for a launched image: writes the
 * command line at NDS_ARGV_CMDLINE_ADDR_{DS,TWL} and the 24-byte header at
 * header_copy_base + NDS_ARGV_OFFSET. Returns the command-line length, or 0 on
 * failure.
 */
u32 loader_stage_argv(const char *name, bool extended_ram);

/*
 * Stage and hand off. Returns a specific nds_load_status_t. With
 * ENABLE_CHAINLOAD_BOOT defined and NDS_LOAD_OK this does not return (the ARM9
 * jumps into the new image); otherwise it performs all staging/IPC work and
 * returns with `out->dry_run` set.
 *
 * `path` is the launch path published through `__system_argv` (e.g.
 * "BOOT.NDS" or "NDS/HELLO.NDS"); pass NULL to fall back to `entry->name`.
 * `loader_stage_and_boot()` is the same thing with `entry->name`.
 */
nds_load_status_t loader_stage_and_boot_path(fat_volume_t *vol,
                                             const fat_file_entry_t *entry,
                                             const char *path,
                                             bool extended_ram,
                                             loader_result_t *out);

nds_load_status_t loader_stage_and_boot(fat_volume_t *vol,
                                        const fat_file_entry_t *entry,
                                        bool extended_ram,
                                        loader_result_t *out);

#endif /* DSI_ARM9_LOADER_H */
