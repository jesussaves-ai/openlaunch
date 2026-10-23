/*
 * include/arm9/installer.h
 *
 * ARM9-side NAND installer engines.
 *
 * Two layers:
 *
 *   1. Injected-I/O engines (always compiled): stream the NAND backup through
 *      a caller-supplied read callback into a caller-supplied sink, hash it
 *      with SHA-256, verify a read-back, and flash a region with a
 *      write-then-read-back pass. These carry no hardware access, so the whole
 *      transfer/verification logic is exercised by tests/test_installer.c.
 *
 *   2. Hardware glue (compiled only under ENABLE_NAND_WRITE_DANGER, the project conventions
 *      §5): the SD FAT-file sink, the ARM7 write guard, and the end-to-end
 *      orchestration. The default build links none of it, so the shipped
 *      bootloader remains physically incapable of writing the eMMC.
 */
#ifndef DSI_ARM9_INSTALLER_H
#define DSI_ARM9_INSTALLER_H

#include "../common/types.h"
#include "../common/sha256.h"
#include "../common/nand.h"
#include "../common/installer.h"
#include "../common/fat_ro.h"

#define INSTALLER_SECTOR_SIZE 512u

/*
 * Bounded sandbox backup. When the authenticated token carries
 * INSTALLER_TOKEN_FLAG_SANDBOX the full-image dump is clamped to this many
 * 512-byte sectors, so NANDBACK.BIN fits on a small (16 MiB) emulator SD image
 * without exhausting FAT clusters. 0 keeps the full NAND dump. Retail/encrypted
 * images are refused by the classifier long before this clamp is reached.
 */
#define INSTALLER_SANDBOX_BACKUP_SECTORS 0x2000u   /* 4 MiB */

/* Callback signatures for the injected-I/O engines. */
typedef bool (*installer_read_fn)(void *ctx, u32 lba, u8 out[INSTALLER_SECTOR_SIZE]);
typedef bool (*installer_write_fn)(void *ctx, u32 lba,
                                   const u8 sector[INSTALLER_SECTOR_SIZE]);
typedef bool (*installer_sink_fn)(void *ctx, const u8 *data, u32 len);
typedef bool (*installer_source_fn)(void *ctx, u32 offset, u8 *out, u32 len);

/* Result of a streamed backup / flash pass. */
typedef struct {
    bool  ok;
    u32   lba;                              /* first sector touched        */
    u32   sectors;                          /* sectors streamed            */
    u32   bytes;                            /* bytes streamed              */
    u8    digest[SHA256_DIGEST_SIZE];       /* SHA-256 of the streamed data*/
    installer_error_t error;                /* INSTALLER_ERR_NONE on success*/
} installer_transfer_t;

/* ---- Injected-I/O engines (host-testable, no hardware) ---------------- */

/*
 * Stream `sectors` 512-byte sectors starting at `start_lba` through `read`,
 * hashing the stream and forwarding it to `sink`. On the ARM9 this reads the
 * eMMC and writes the SD backup file. Fails closed on any read/sink error.
 */
installer_error_t installer_backup_stream(installer_read_fn read, void *read_ctx,
                                          installer_sink_fn sink, void *sink_ctx,
                                          u32 start_lba, u32 sectors,
                                          installer_transfer_t *out);

/*
 * Re-read the same range and compare its SHA-256 against `expected`. Used to
 * prove the backup matches live NAND (and, with a different `expected` + a
 * write pass, to prove a flash read-back).
 */
installer_error_t installer_backup_verify(installer_read_fn read, void *read_ctx,
                                          const u8 expected[SHA256_DIGEST_SIZE],
                                          u32 start_lba, u32 sectors);

/*
 * Flash `sectors` sectors starting at `start_lba`: for each sector, source up
 * to 512 bytes through `src` (zero-padded past the payload), write it, then
 * read it back and compare byte-for-byte. The payload digest is accumulated
 * and checked against `expected` (pass NULL to skip the digest check).
 */
installer_error_t installer_flash_region(installer_read_fn read, void *read_ctx,
                                         installer_write_fn write, void *write_ctx,
                                         installer_source_fn src, void *src_ctx,
                                         u32 start_lba, u32 sectors,
                                         const u8 expected[SHA256_DIGEST_SIZE],
                                         installer_transfer_t *out);

/* ---- Read-only helpers (always compiled) ------------------------------ */

/* Return a source callback that streams from a FAT file entry. */
typedef struct {
    fat_volume_t     *vol;
    fat_file_entry_t  entry;
} installer_sd_source_t;

void installer_sd_source_init(installer_sd_source_t *s, fat_volume_t *vol,
                              const fat_file_entry_t *entry);

/* Fill `out` with 512 bytes of the payload at byte `offset` (zero past EOF). */
bool installer_sd_source_read(void *ctx, u32 offset, u8 *out, u32 len);

/*
 * SD backup persistence verification (always compiled, host-testable).
 *
 * `installer_backup_verify()` only re-reads the live NAND, so a dropped or
 * truncated SD write is invisible to it. This helper re-opens NANDBACK.BIN on
 * the mounted SD volume and proves the backup really persisted: the file must
 * exist and be a regular file, its on-disk size must equal `expected_bytes`,
 * and the SHA-256 of its contents must equal `expected_digest` (the digest of
 * the NAND data streamed during BACKUP). Returns INSTALLER_ERR_NONE only when
 * all three hold; a missing/short/corrupt file is INSTALLER_ERR_BACKUP_MISMATCH.
 */
installer_error_t installer_backup_verify_sd(
    fat_volume_t *vol,
    const u8 expected_digest[SHA256_DIGEST_SIZE],
    u32 expected_bytes, installer_transfer_t *out);

/* ---- Hardware glue (ENABLE_NAND_WRITE_DANGER only) -------------------- */

/* Root-directory file names used by the installer (strict 8.3). */
#define INSTALLER_TOKEN_FILE   "INSTALL.KEY"
#define INSTALLER_PAYLOAD_FILE "OLBOOT.NDS"
#define INSTALLER_BACKUP_FILE  "NANDBACK.BIN"

#ifdef ENABLE_NAND_WRITE_DANGER

/*
 * Create `nand_backup.bin` in the root of the mounted SD volume `vol` and
 * stream the full NAND image into it, hashing as it goes. On any failure the
 * partially written file is aborted (its clusters freed) and disarmed.
 */
installer_error_t installer_backup_to_sd(fat_volume_t *vol,
                                         u32 start_lba, u32 sectors,
                                         installer_transfer_t *out);

/*
 * Runtime write guard, layered on top of the compile-time gate. The ARM7
 * refuses every eMMC write until it has been armed with a non-zero id derived
 * from the authenticated installer token; the guard is disarmed on completion
 * and on any error.
 */
installer_error_t installer_guard_arm(u32 auth_id);
installer_error_t installer_guard_disarm(void);

/*
 * Arm the guard, write `target_sectors` sectors from the `src` callback to
 * `target_lba` (zero-padding past the payload), verify the read-back against
 * `expected`, then always disarm the guard before returning.
 */
installer_error_t installer_flash_to_nand(u32 auth_id,
                                          installer_source_fn src, void *src_ctx,
                                          u32 target_lba, u32 target_sectors,
                                          const u8 expected[SHA256_DIGEST_SIZE],
                                          installer_transfer_t *out);

#endif /* ENABLE_NAND_WRITE_DANGER */

/* ---- High-level orchestration (always compiled) ----------------------- */

/* Short on-screen summary produced by installer_run(). */
typedef struct {
    installer_state_t state;     /* terminal state                         */
    installer_state_t failed_state; /* state reached when ABORTED          */
    installer_error_t error;     /* INSTALLER_ERR_NONE on success          */
    bool write_enabled;          /* compiled with ENABLE_NAND_WRITE_DANGER */
    bool token_found;            /* INSTALL.KEY was read from the SD root  */
    bool backup_ok;              /* the backup was written + verified      */
    bool flashed;                /* the target region was written + verified*/
    char detail[40];             /* extra diagnostic text                  */
} installer_report_t;

/*
 * Run the complete installer flow for the `[1] Install to NAND` menu item:
 * pre-flight, mandatory backup + verification, guarded flash, read-back
 * verification, and unconditional guard release. Always fills `*out`; never
 * returns on a partially-written state without first disarming the guard.
 *
 * In the default (locked) build this immediately reports
 * INSTALLER_STATE_ABORTED / INSTALLER_ERR_DISABLED.
 */
void installer_run(installer_report_t *out);

#endif /* DSI_ARM9_INSTALLER_H */
