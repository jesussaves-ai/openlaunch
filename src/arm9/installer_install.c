/*
 * src/arm9/installer_install.c
 *
 * Flash + read-back verification engine and the end-to-end installer flow.
 * See include/arm9/installer.h.
 *
 * The flash engine is a pure injected-I/O function (host-tested). Everything
 * that actually reaches the eMMC -- the ARM7 write guard and the orchestration
 * -- is compiled only under ENABLE_NAND_WRITE_DANGER (AGENTS.md §5). In the
 * default build installer_run() reports the locked state and refuses.
 */
#include "arm9/installer.h"
#include "common/mem.h"

/* Shared-RAM staging buffers (never DTCM). */
static u8 s_flash_payload[INSTALLER_SECTOR_SIZE];
static u8 s_flash_readback[INSTALLER_SECTOR_SIZE];

/* ---- Flash + read-back engine (host-testable) ------------------------- */

installer_error_t installer_flash_region(installer_read_fn read, void *read_ctx,
                                         installer_write_fn write, void *write_ctx,
                                         installer_source_fn src, void *src_ctx,
                                         u32 start_lba, u32 sectors,
                                         const u8 expected[SHA256_DIGEST_SIZE],
                                         installer_transfer_t *out)
{
    sha256_ctx_t sha;
    u32 i;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
        out->error = INSTALLER_ERR_WRITE_FAILED;
    }
    if (read == NULL || write == NULL || src == NULL || sectors == 0u) {
        return INSTALLER_ERR_WRITE_FAILED;
    }

    sha256_init(&sha);
    for (i = 0u; i < sectors; i++) {
        if (!src(src_ctx, i * INSTALLER_SECTOR_SIZE, s_flash_payload,
                 INSTALLER_SECTOR_SIZE)) {
            return INSTALLER_ERR_WRITE_FAILED;
        }
        sha256_update(&sha, s_flash_payload, INSTALLER_SECTOR_SIZE);

        if (!write(write_ctx, start_lba + i, s_flash_payload)) {
            return INSTALLER_ERR_WRITE_FAILED;
        }
        /* Immediate read-back: the sector must match what we just wrote. */
        if (!read(read_ctx, start_lba + i, s_flash_readback)) {
            return INSTALLER_ERR_WRITE_FAILED;
        }
        if (dsi_memcmp(s_flash_payload, s_flash_readback,
                       INSTALLER_SECTOR_SIZE) != 0) {
            return INSTALLER_ERR_VERIFY_MISMATCH;
        }
        installer_progress_step(INSTALLER_STATE_WRITE, i + 1u, sectors, "write");
    }
    sha256_final(&sha, out->digest);

    /* The per-sector read-back already happened above; surface the final
     * digest comparison as the distinct VERIFY_WRITE stage. */
    installer_progress_state(INSTALLER_STATE_VERIFY_WRITE, "compare digest");

    if (expected != NULL && !sha256_digest_equal(out->digest, expected)) {
        return INSTALLER_ERR_VERIFY_MISMATCH;
    }

    out->ok = true;
    out->lba = start_lba;
    out->sectors = sectors;
    out->bytes = sectors * INSTALLER_SECTOR_SIZE;
    out->error = INSTALLER_ERR_NONE;
    return INSTALLER_ERR_NONE;
}

/* ---- Report helper (always compiled) ---------------------------------- */

static void installer_copy_str(char *dst, u32 cap, const char *src)
{
    u32 i = 0u;

    if (dst == NULL || cap == 0u) {
        return;
    }
    if (src != NULL) {
        while (src[i] != '\0' && i + 1u < cap) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

#ifndef ENABLE_NAND_WRITE_DANGER

void installer_run(installer_report_t *out)
{
    if (out == NULL) {
        return;
    }
    dsi_memset(out, 0, sizeof(*out));
    out->state = INSTALLER_STATE_ABORTED;
    out->failed_state = INSTALLER_STATE_PREFLIGHT;
    out->error = INSTALLER_ERR_DISABLED;
    out->write_enabled = false;
    installer_copy_str(out->detail, sizeof(out->detail),
                       "NAND write path not built");
    /* Let the modal UI show the PREFLIGHT -> ABORTED transition even here. */
    installer_progress_state(INSTALLER_STATE_PREFLIGHT, "preflight");
    installer_progress_state(INSTALLER_STATE_ABORTED,
                             installer_error_text(out->error));
}

#else /* ENABLE_NAND_WRITE_DANGER */

#include "arm9/ipc.h"
#include "arm9/nand_boot.h"
#include "arm9/storage.h"
#include "common/fat_rw.h"
#include "common/ipc_protocol.h"

#define INSTALLER_IPC_TIMEOUT 0x00400000u

/* ---- Runtime write guard (ARM7 handshake) ------------------------------ */

installer_error_t installer_guard_arm(u32 auth_id)
{
    u32 args[IPC_REQ_NAND_GUARD_WORDS];
    u32 reply[2];

    if (auth_id == 0u) {
        return INSTALLER_ERR_UNLOCK_FAILED;
    }
    args[0] = auth_id;
    if (arm9_ipc_query_args(IPC_CH_SYSTEM, IPC_CMD_NAND_GUARD, args, 1u,
                            reply, ARRAY_SIZE(reply),
                            INSTALLER_IPC_TIMEOUT) < ARRAY_SIZE(reply)) {
        return INSTALLER_ERR_UNLOCK_FAILED;
    }
    return (reply[1] & IPC_IO_FLAG_OK) ? INSTALLER_ERR_NONE
                                       : INSTALLER_ERR_UNLOCK_FAILED;
}

installer_error_t installer_guard_disarm(void)
{
    u32 args[IPC_REQ_NAND_GUARD_WORDS];
    u32 reply[2];

    args[0] = 0u;
    if (arm9_ipc_query_args(IPC_CH_SYSTEM, IPC_CMD_NAND_GUARD, args, 1u,
                            reply, ARRAY_SIZE(reply),
                            INSTALLER_IPC_TIMEOUT) < ARRAY_SIZE(reply)) {
        return INSTALLER_ERR_UNLOCK_FAILED;
    }
    return (reply[1] & IPC_IO_FLAG_OK) ? INSTALLER_ERR_NONE
                                       : INSTALLER_ERR_UNLOCK_FAILED;
}

/* ---- Hardware adapters ------------------------------------------------- */

static bool installer_emu_read(void *ctx, u32 lba, u8 out[INSTALLER_SECTOR_SIZE])
{
    (void)ctx;
    return arm9_nand_read_sectors(lba, out, 1u);
}

static bool installer_emu_write(void *ctx, u32 lba,
                                const u8 sector[INSTALLER_SECTOR_SIZE])
{
    (void)ctx;
    return arm9_storage_write_sectors(IPC_STORAGE_DEV_EMMC, lba, sector, 1u);
}

installer_error_t installer_flash_to_nand(u32 auth_id,
                                          installer_source_fn src, void *src_ctx,
                                          u32 target_lba, u32 target_sectors,
                                          const u8 expected[SHA256_DIGEST_SIZE],
                                          installer_transfer_t *out)
{
    installer_error_t err;

    err = installer_guard_arm(auth_id);
    if (err != INSTALLER_ERR_NONE) {
        return err;
    }

    err = installer_flash_region(installer_emu_read, NULL,
                                 installer_emu_write, NULL,
                                 src, src_ctx,
                                 target_lba, target_sectors, expected, out);

    /* Always release the guard, whatever happened. */
    (void)installer_guard_disarm();
    return err;
}

/* ---- End-to-end flow --------------------------------------------------- */

/* Static state lives in shared Main RAM (.bss). */
static u8 s_nand_lba0[INSTALLER_SECTOR_SIZE];
static u8 s_token_buf[INSTALLER_TOKEN_SIZE];

/*
 * Record an abort: stamp the terminal state, the stage that failed (for the
 * error screen), the error and the diagnostic, then notify the progress hook.
 */
static void installer_fail(installer_report_t *out, installer_state_t stage,
                           installer_error_t error, const char *detail)
{
    out->state = INSTALLER_STATE_ABORTED;
    out->failed_state = stage;
    out->error = error;
    installer_copy_str(out->detail, sizeof(out->detail), detail);
    installer_progress_state(INSTALLER_STATE_ABORTED,
                             installer_error_text(error));
}

void installer_run(installer_report_t *out)
{
    fat_volume_t vol;
    fat_file_entry_t token_entry;
    fat_file_entry_t payload_entry;
    installer_sd_source_t src;
    nand_layout_t layout;
    installer_token_t token;
    installer_region_t region;
    installer_request_t req;
    installer_decision_t decision;
    installer_transfer_t xfer;
    installer_error_t err;
    u32 token_len = 0u;
    u32 auth_id;

    if (out == NULL) {
        return;
    }
    dsi_memset(out, 0, sizeof(*out));
    out->state = INSTALLER_STATE_IDLE;
    out->failed_state = INSTALLER_STATE_IDLE;
    out->error = INSTALLER_ERR_NONE;
    out->write_enabled = true;
    installer_copy_str(out->detail, sizeof(out->detail), "starting");

    installer_progress_state(INSTALLER_STATE_PREFLIGHT, "preflight");

    /* 1. Mount the SD card (backup destination + package host). */
    if (!arm9_storage_mount(&vol, IPC_STORAGE_DEV_SD)) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_NO_SD,
                       "no SD/FAT");
        return;
    }

    /* 2. Read + validate the NAND partition table before anything else. */
    if (!arm9_nand_read_sectors(0u, s_nand_lba0, 1u) ||
        nand_validate_layout(s_nand_lba0, NAND_MAX_SECTORS, &layout) !=
            NAND_LAYOUT_OK) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_BAD_LAYOUT,
                       "bad NAND layout");
        return;
    }

    /* 3. Load and authenticate the signed installer token. */
    if (!fat_find_path(&vol, INSTALLER_TOKEN_FILE, &token_entry) ||
        token_entry.is_dir ||
        !fat_read_file(&vol, &token_entry, s_token_buf, sizeof(s_token_buf),
                       &token_len) ||
        installer_token_parse(s_token_buf, token_len, &token) !=
            INSTALLER_TOKEN_OK ||
        installer_token_verify(&token, NULL, 0u) != INSTALLER_TOKEN_OK) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_BAD_TOKEN,
                       "bad/missing token");
        return;
    }
    out->token_found = true;

    /* 4. Resolve the target region and refuse anything unsafe. */
    err = installer_plan_region(&token, &layout, &region);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, err,
                       installer_error_text(err));
        return;
    }

    /* 5. The payload image must exist on the SD root. */
    if (!fat_find_path(&vol, INSTALLER_PAYLOAD_FILE, &payload_entry) ||
        payload_entry.is_dir) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_BAD_TOKEN,
                       "no payload file");
        return;
    }

    /* 6. Pre-flight gate (all conditions now known). */
    dsi_memset(&req, 0, sizeof(req));
    req.write_enabled = true;
    req.sd_present = true;
    req.token_valid = true;
    req.layout_ok = true;
    req.backup_verified = false;
    decision = installer_preflight(&req);
    if (!decision.proceed) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, decision.error,
                       installer_error_text(decision.error));
        return;
    }

    /* 7. Mandatory full backup, then verify it against live NAND. */
    installer_progress_state(INSTALLER_STATE_BACKUP, "backup");
    out->state = INSTALLER_STATE_BACKUP;
    err = installer_backup_to_sd(&vol, 0u, layout.total_backup_sectors, &xfer);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_BACKUP, err, "backup failed");
        return;
    }
    installer_progress_state(INSTALLER_STATE_VERIFY_BACKUP, "verify backup");
    out->state = INSTALLER_STATE_VERIFY_BACKUP;
    err = installer_backup_verify(installer_emu_read, NULL, xfer.digest, 0u,
                                  layout.total_backup_sectors);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_VERIFY_BACKUP, err,
                       "backup mismatch");
        return;
    }
    out->backup_ok = true;

    /* 8. Guarded flash + read-back verify (guard is released inside). */
    auth_id = dsi_read_le32(token.payload_sha256);
    if (auth_id == 0u) {
        auth_id = 0x00000001u;
    }
    installer_sd_source_init(&src, &vol, &payload_entry);

    installer_progress_state(INSTALLER_STATE_UNLOCK, "arm write guard");
    out->state = INSTALLER_STATE_UNLOCK;
    installer_progress_state(INSTALLER_STATE_WRITE, "write NAND");
    out->state = INSTALLER_STATE_WRITE;
    err = installer_flash_to_nand(auth_id, installer_sd_source_read, &src,
                                  region.lba, region.count,
                                  token.payload_sha256, &xfer);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_WRITE, err,
                       (err == INSTALLER_ERR_VERIFY_MISMATCH)
                           ? "read-back mismatch"
                           : "flash failed");
        return;
    }

    out->flashed = true;
    out->state = INSTALLER_STATE_DONE;
    out->failed_state = INSTALLER_STATE_DONE;
    out->error = INSTALLER_ERR_NONE;
    installer_copy_str(out->detail, sizeof(out->detail), "installed OK");
    installer_progress_state(INSTALLER_STATE_DONE, "installed OK");
}

#endif /* ENABLE_NAND_WRITE_DANGER */
