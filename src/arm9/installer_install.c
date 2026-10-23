/*
 * src/arm9/installer_install.c
 *
 * Flash + read-back verification engine and the end-to-end installer flow.
 * See include/arm9/installer.h.
 *
 * The flash engine is a pure injected-I/O function (host-tested). Everything
 * that actually reaches the eMMC -- the ARM7 write guard and the orchestration
 * -- is compiled only under ENABLE_NAND_WRITE_DANGER (the project conventions). In the
 * default build installer_run() reports the locked state and refuses.
 */
#include "arm9/installer.h"
#include "common/mem.h"

/*
 * The opt-in retail write path arms raw eMMC writes on an AES-CTR encrypted
 * retail console. It is meaningless (and unsafe) without the write path itself,
 * so require both flags. DANGER: see docs/hardware_notes.md 34.11.
 */
#if defined(ENABLE_RETAIL_NAND_WRITE) && !defined(ENABLE_NAND_WRITE_DANGER)
#error "ENABLE_RETAIL_NAND_WRITE requires ENABLE_NAND_WRITE_DANGER"
#endif

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
static u8 s_nand_footer[INSTALLER_SECTOR_SIZE];
static u8 s_token_buf[INSTALLER_TOKEN_SIZE];

/*
 * The mounted SD volume. It carries a 512-byte sector buffer that the ARM7
 * fills directly over IPC, so it MUST live in shared Main RAM - never on the
 * ARM9 stack, which is in DTCM (0x0B000000) and is rejected by
 * ipc_addr_is_shared_ram(). A stack-local fat_volume_t made the installer fail
 * at 1/6 PREFLIGHT with "no SD/FAT" (see the project conventions 3 / storage.h).
 */
static fat_volume_t s_installer_vol;

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
    fat_volume_t *vol = &s_installer_vol;
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
    nand_image_kind_t image_kind;
    u32 token_len = 0u;
    u32 auth_id;
    u32 backup_sectors;
    bool lba0_ok;
    bool footer_ok;
    bool token_ok;
    bool layout_ok = false;
    bool sandbox;
    bool retail;
    bool retail_armed = false;

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

    /*
     * 0. The mount volume's sector buffer is filled directly by the ARM7 over
     *    IPC, so it MUST live in shared Main RAM (0x02000000..0x02400000) and
     *    never on the ARM9 stack, which is in DTCM and is rejected by
     *    ipc_addr_is_shared_ram(). Verify that invariant explicitly.
     */
    if (!ipc_addr_is_shared_ram((u32)(uintptr_t)vol, sizeof(*vol))) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_NO_SD,
                       "installer volume not in shared RAM");
        return;
    }

    /* 1. Mount the SD card (backup destination + package host). */
    if (!arm9_storage_mount(vol, IPC_STORAGE_DEV_SD)) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_NO_SD,
                       "no SD/FAT");
        return;
    }

    /*
     * 2. Read and classify the eMMC image before anything else. A retail image
     *    is AES-CTR encrypted with a console-unique key and validated by a
     *    signed/TMD boot chain, so raw plaintext staging can never be valid.
     */
    lba0_ok = arm9_nand_read_sectors(0u, s_nand_lba0, 1u);
    footer_ok = arm9_nand_read_sectors(NAND_FOOTER_LBA, s_nand_footer, 1u);
    image_kind = nand_classify_image(lba0_ok ? s_nand_lba0 : NULL,
                                     footer_ok ? s_nand_footer : NULL);
    if (image_kind != NAND_IMAGE_ENCRYPTED_RETAIL && lba0_ok) {
        layout_ok = (nand_validate_layout(s_nand_lba0, NAND_MAX_SECTORS,
                                          &layout) == NAND_LAYOUT_OK);
    }

    /* 3. Load and authenticate the signed installer token. */
    token_ok = fat_find_path(vol, INSTALLER_TOKEN_FILE, &token_entry) &&
               !token_entry.is_dir &&
               fat_read_file(vol, &token_entry, s_token_buf, sizeof(s_token_buf),
                             &token_len) &&
               installer_token_parse(s_token_buf, token_len, &token) ==
                   INSTALLER_TOKEN_OK &&
               installer_token_verify(&token, NULL, 0u) == INSTALLER_TOKEN_OK;
    out->token_found = token_ok;

    retail = (image_kind == NAND_IMAGE_ENCRYPTED_RETAIL);
#ifdef ENABLE_RETAIL_NAND_WRITE
    /*
     * DANGER: arm the raw retail write only when the build flag is present,
     * the token explicitly authorises retail, and the target is inside the
     * hardcoded boot-safe window (the encrypted MBR cannot provide a layout,
     * so this floor is what keeps stage1/stage2 - and an existing Unlaunch -
     * intact). Any one condition failing keeps the retail image refused.
     */
    if (retail && token_ok &&
        (token.flags & INSTALLER_TOKEN_FLAG_RETAIL) != 0u &&
        nand_retail_region_is_safe(token.target_lba, token.target_sectors)) {
        retail_armed = true;
    }
#endif

    /*
     * 4. Pre-flight gate. Every condition is now known; the gate refuses a
     *    retail image with the specific INSTALLER_ERR_ENCRYPTED_NAND status so
     *    the UI can explain why raw staging is unsafe there.
     */
    dsi_memset(&req, 0, sizeof(req));
    req.write_enabled = true;
    req.sd_present = true;
    req.retail_encrypted = retail;
    req.retail_write_armed = retail_armed;
    req.layout_ok = layout_ok;
    req.token_valid = token_ok;
    req.backup_verified = false;
    decision = installer_preflight(&req);
    if (!decision.proceed) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, decision.error,
                       installer_error_text(decision.error));
        return;
    }

    /* 5. Resolve the target region and require the payload on the SD root. */
    if (retail_armed) {
        /* No readable MBR: trust the authenticated token, already bounded by
         * nand_retail_region_is_safe() above. */
        region.lba = token.target_lba;
        region.count = token.target_sectors;
    } else {
        err = installer_plan_region(&token, &layout, &region);
        if (err != INSTALLER_ERR_NONE) {
            installer_fail(out, INSTALLER_STATE_PREFLIGHT, err,
                           installer_error_text(err));
            return;
        }
    }
    if (!fat_find_path(vol, INSTALLER_PAYLOAD_FILE, &payload_entry) ||
        payload_entry.is_dir) {
        installer_fail(out, INSTALLER_STATE_PREFLIGHT, INSTALLER_ERR_BAD_TOKEN,
                       "no payload file");
        return;
    }

    /*
     * 6. Mandatory backup. Verify it twice: against live NAND (stability) and
     *    by reading the SD file back (persistence). A sandbox package (token
     *    flag) bounds the dump so it fits a small emulator SD image; a retail
     *    console dumps the whole chip.
     */
    sandbox = (token.flags & INSTALLER_TOKEN_FLAG_SANDBOX) != 0u;
    if (retail_armed) {
        backup_sectors = NAND_RETAIL_USABLE_SECTORS;
    } else {
        backup_sectors = layout.total_backup_sectors;
    }
    if (sandbox && backup_sectors > INSTALLER_SANDBOX_BACKUP_SECTORS) {
        backup_sectors = INSTALLER_SANDBOX_BACKUP_SECTORS;
    }

    installer_progress_state(INSTALLER_STATE_BACKUP, "backup");
    out->state = INSTALLER_STATE_BACKUP;
    err = installer_backup_to_sd(vol, 0u, backup_sectors, &xfer);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_BACKUP, err, "backup failed");
        return;
    }

    installer_progress_state(INSTALLER_STATE_VERIFY_BACKUP, "verify backup");
    out->state = INSTALLER_STATE_VERIFY_BACKUP;
    /* 6a. The streamed dump must still match live NAND. */
    err = installer_backup_verify(installer_emu_read, NULL, xfer.digest, 0u,
                                  backup_sectors);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_VERIFY_BACKUP, err,
                       "backup mismatch");
        return;
    }
    /* 6b. The backup must actually persist on the SD card (size + SHA-256). */
    err = installer_backup_verify_sd(vol, xfer.digest, xfer.bytes, NULL);
    if (err != INSTALLER_ERR_NONE) {
        installer_fail(out, INSTALLER_STATE_VERIFY_BACKUP, err,
                       "backup not persisted on SD");
        return;
    }
    out->backup_ok = true;

    /* 7. Guarded flash + read-back verify (guard is released inside). */
    auth_id = dsi_read_le32(token.payload_sha256);
    if (auth_id == 0u) {
        auth_id = 0x00000001u;
    }
    installer_sd_source_init(&src, vol, &payload_entry);

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
