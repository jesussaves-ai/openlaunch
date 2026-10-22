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
#include "../common/dldi.h"
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

/*
 * Bounded window used by loader_detect_dldi(): the ARM9 prefix streamed into a
 * .bss scratch buffer for the verify/dry-run inspector. Large enough for a
 * header-proximate DLDI stub without staging the whole image.
 */
#define LOADER_DLDI_SCAN_MAX    0x00004000u   /* 16 KiB */

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

    /* DLDI section detection (and optional in-RAM patching). */
    bool dldi_present;     /* a valid DLDI stub was found in the ARM9      */
    bool dldi_patched;     /* it was replaced by the built-in RO driver    */
    u32  dldi_alloc_kb;    /* reserved block size in KiB                   */
    char dldi_io_type[DLDI_IO_TYPE_LEN + 1u]; /* 4-char tag + NUL       */
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

/*
 * Bounded DLDI detection for the verify/dry-run inspector: stream up to
 * LOADER_DLDI_SCAN_MAX bytes of the ARM9 segment through the delegated FAT
 * reader and scan that prefix for a valid DLDI stub. Fills `*out` when present
 * (the string fields of a zeroed `dldi_info_t` otherwise).
 *
 * The scan is bounded because the inspector reads into a fixed .bss window
 * rather than staging the whole image; the deterministic payload keeps its
 * stub at offset 0. Returns true only for a valid, in-range stub.
 */
bool loader_detect_dldi(fat_volume_t *vol, const fat_file_entry_t *entry,
                        const nds_header_t *hdr, dldi_info_t *out);

/*
 * Read and decode the optional NDS banner referenced by `hdr->banner_offset`.
 *
 * Streams the 0x840-byte banner block through the delegated FAT reader into a
 * fixed .bss window and runs nds_banner_parse(). Returns true when the banner
 * block was read and parsed (even if its version/CRC is invalid); false when
 * there is no banner (`banner_offset == 0`), it does not fit the file, or the
 * read fails. `*out` is zeroed first, so callers may query `version_ok` /
 * `crc_ok` / title safely either way.
 */
bool loader_read_banner(fat_volume_t *vol, const fat_file_entry_t *entry,
                        const nds_header_t *hdr, nds_banner_info_t *out);

#endif /* DSI_ARM9_LOADER_H */
