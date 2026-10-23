/*
 * src/common/installer.c
 *
 * Pure installer policy. See include/common/installer.h.
 *
 * No hardware access and no allocation: the token codec works on a caller
 * buffer, the state machine is a lookup, and the pre-flight gate is a strict
 * boolean cascade. This is the part of the installer that must be provably
 * correct, so it is exercised exhaustively by tests/test_installer.c.
 */
#include "common/installer.h"
#include "common/mem.h"

/*
 * Build-time authentication key for the signed installer package.
 *
 * Overridable with -DINSTALLER_AUTH_KEY="..." (a C string) by the packaging
 * tooling; the default is a documented, non-secret development key so the
 * pipeline is reproducible. This gate hardens the *runtime* authorisation; the
 * authoritative safety boundary remains the ENABLE_NAND_WRITE_DANGER build
 * flag (the project conventions).
 */
#ifndef INSTALLER_AUTH_KEY
#define INSTALLER_AUTH_KEY "OpenLaunch-NAND-installer-dev-key"
#endif

static const char *installer_auth_key(void)
{
    return INSTALLER_AUTH_KEY;
}

/* ---- Token codec ------------------------------------------------------ */

const char *installer_token_status_text(installer_token_status_t status)
{
    switch (status) {
    case INSTALLER_TOKEN_OK:            return "token OK";
    case INSTALLER_TOKEN_ERR_NULL:      return "null token";
    case INSTALLER_TOKEN_ERR_SIZE:      return "wrong token size";
    case INSTALLER_TOKEN_ERR_MAGIC:     return "bad token magic";
    case INSTALLER_TOKEN_ERR_VERSION:   return "unsupported token version";
    case INSTALLER_TOKEN_ERR_SIGNATURE: return "token signature mismatch";
    case INSTALLER_TOKEN_ERR_RANGE:     return "token target out of range";
    default:                            return "unknown token error";
    }
}

installer_token_status_t installer_token_parse(const u8 *buf, u32 len,
                                               installer_token_t *out)
{
    u32 i;

    if (buf == NULL || out == NULL) {
        return INSTALLER_TOKEN_ERR_NULL;
    }
    if (len != INSTALLER_TOKEN_SIZE) {
        return INSTALLER_TOKEN_ERR_SIZE;
    }

    dsi_memset(out, 0, sizeof(*out));
    out->magic = dsi_read_le32(buf + 0x00);
    out->version = dsi_read_le16(buf + 0x04);
    out->flags = dsi_read_le16(buf + 0x06);
    out->target_lba = dsi_read_le32(buf + 0x08);
    out->target_sectors = dsi_read_le32(buf + 0x0C);
    out->payload_sectors = dsi_read_le32(buf + 0x10);
    dsi_memcpy(out->payload_sha256, buf + 0x14, SHA256_DIGEST_SIZE);
    dsi_memcpy(out->signature, buf + 0x40, SHA256_DIGEST_SIZE);

    if (out->magic != INSTALLER_TOKEN_MAGIC) {
        return INSTALLER_TOKEN_ERR_MAGIC;
    }
    if (out->version != INSTALLER_TOKEN_VERSION) {
        return INSTALLER_TOKEN_ERR_VERSION;
    }
    /* The reserved field must be all zero: reject malformed/hand-edited
     * packages rather than silently accepting trailing garbage. */
    for (i = 0x34u; i < 0x40u; i++) {
        if (buf[i] != 0u) {
            return INSTALLER_TOKEN_ERR_RANGE;
        }
    }
    if (out->target_sectors == 0u || out->payload_sectors == 0u) {
        return INSTALLER_TOKEN_ERR_RANGE;
    }

    return INSTALLER_TOKEN_OK;
}

static void installer_u16_store(u8 *p, u16 v)
{
    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)(v >> 8);
}

static void installer_token_sign_bytes(const installer_token_t *token,
                                       u8 out[INSTALLER_TOKEN_SIZE])
{
    /* Serialise the signed prefix [0, 0x40) plus a zeroed reserved field. */
    dsi_memset(out, 0, INSTALLER_TOKEN_SIZE);
    dsi_write_le32(out + 0x00, token->magic);
    installer_u16_store(out + 0x04, token->version);
    installer_u16_store(out + 0x06, token->flags);
    dsi_write_le32(out + 0x08, token->target_lba);
    dsi_write_le32(out + 0x0C, token->target_sectors);
    dsi_write_le32(out + 0x10, token->payload_sectors);
    dsi_memcpy(out + 0x14, token->payload_sha256, SHA256_DIGEST_SIZE);
}

void installer_token_encode(const installer_token_t *token,
                            const u8 *key, u32 key_len,
                            u8 out[INSTALLER_TOKEN_SIZE])
{
    if (token == NULL || out == NULL) {
        return;
    }
    if (key == NULL) {
        key = (const u8 *)installer_auth_key();
        key_len = (u32)dsi_strlen(installer_auth_key());
    }

    installer_token_sign_bytes(token, out);
    hmac_sha256(key, key_len, out, INSTALLER_TOKEN_SIGNED_LEN,
                out + INSTALLER_TOKEN_SIGNED_LEN);
}

installer_token_status_t installer_token_verify(const installer_token_t *token,
                                                const u8 *key, u32 key_len)
{
    u8 signed_bytes[INSTALLER_TOKEN_SIZE];
    u8 expected[SHA256_DIGEST_SIZE];
    bool ok;

    if (token == NULL) {
        return INSTALLER_TOKEN_ERR_NULL;
    }
    if (key == NULL) {
        key = (const u8 *)installer_auth_key();
        key_len = (u32)dsi_strlen(installer_auth_key());
    }

    installer_token_sign_bytes(token, signed_bytes);
    hmac_sha256(key, key_len, signed_bytes, INSTALLER_TOKEN_SIGNED_LEN, expected);

    ok = sha256_digest_equal(expected, token->signature);
    dsi_memset(expected, 0, sizeof(expected));
    dsi_memset(signed_bytes, 0, sizeof(signed_bytes));

    return ok ? INSTALLER_TOKEN_OK : INSTALLER_TOKEN_ERR_SIGNATURE;
}

installer_token_status_t installer_token_check_range(const installer_token_t *t,
                                                     const nand_layout_t *layout)
{
    u32 target_end;
    u32 volume_end;

    if (t == NULL || layout == NULL) {
        return INSTALLER_TOKEN_ERR_NULL;
    }
    if (t->target_sectors == 0u) {
        return INSTALLER_TOKEN_ERR_RANGE;
    }
    if (nand_region_is_protected(layout, t->target_lba, t->target_sectors)) {
        return INSTALLER_TOKEN_ERR_RANGE;
    }
    if (t->target_lba > 0xFFFFFFFFu - t->target_sectors) {
        return INSTALLER_TOKEN_ERR_RANGE;
    }
    target_end = t->target_lba + t->target_sectors;
    volume_end = layout->partition_lba + layout->partition_sectors;
    if (volume_end < layout->partition_lba) {
        volume_end = 0xFFFFFFFFu;   /* overflow: treat as unbounded */
    }
    if (target_end > volume_end) {
        return INSTALLER_TOKEN_ERR_RANGE;
    }
    return INSTALLER_TOKEN_OK;
}

/* ---- State machine ---------------------------------------------------- */

const char *installer_state_text(installer_state_t state)
{
    switch (state) {
    case INSTALLER_STATE_IDLE:          return "idle";
    case INSTALLER_STATE_PREFLIGHT:     return "preflight";
    case INSTALLER_STATE_BACKUP:        return "backup";
    case INSTALLER_STATE_VERIFY_BACKUP: return "verify backup";
    case INSTALLER_STATE_UNLOCK:        return "unlock";
    case INSTALLER_STATE_WRITE:         return "write";
    case INSTALLER_STATE_VERIFY_WRITE:  return "verify write";
    case INSTALLER_STATE_DONE:          return "done";
    case INSTALLER_STATE_ABORTED:       return "aborted";
    default:                            return "unknown";
    }
}

installer_state_t installer_advance(installer_state_t state, bool step_ok)
{
    if (!step_ok) {
        /* IDLE has not started; a failure there simply stays idle. */
        return (state == INSTALLER_STATE_IDLE) ? INSTALLER_STATE_IDLE
                                               : INSTALLER_STATE_ABORTED;
    }
    switch (state) {
    case INSTALLER_STATE_IDLE:          return INSTALLER_STATE_PREFLIGHT;
    case INSTALLER_STATE_PREFLIGHT:     return INSTALLER_STATE_BACKUP;
    case INSTALLER_STATE_BACKUP:        return INSTALLER_STATE_VERIFY_BACKUP;
    case INSTALLER_STATE_VERIFY_BACKUP: return INSTALLER_STATE_UNLOCK;
    case INSTALLER_STATE_UNLOCK:        return INSTALLER_STATE_WRITE;
    case INSTALLER_STATE_WRITE:         return INSTALLER_STATE_VERIFY_WRITE;
    case INSTALLER_STATE_VERIFY_WRITE:  return INSTALLER_STATE_DONE;
    case INSTALLER_STATE_DONE:
    case INSTALLER_STATE_ABORTED:
    default:                            return state;   /* terminal */
    }
}

/* ---- Progress observation --------------------------------------------- */

static installer_progress_fn s_progress_fn;
static void *s_progress_ctx;

void installer_set_progress_hook(installer_progress_fn fn, void *ctx)
{
    s_progress_fn = fn;
    s_progress_ctx = ctx;
}

u8 installer_percent(u32 done, u32 total)
{
    u64 scaled;

    if (total == 0u || done == 0u) {
        return 0u;
    }
    if (done >= total) {
        return 100u;
    }
    /* 64-bit intermediate so a full-NAND sector count cannot overflow. */
    scaled = ((u64)done * 100u) / (u64)total;
    return (u8)scaled;
}

void installer_progress_step(installer_state_t state, u32 done, u32 total,
                             const char *message)
{
    installer_progress_t p;

    if (s_progress_fn == NULL) {
        return;
    }
    p.state = state;
    p.done = done;
    p.total = total;
    p.percent = installer_percent(done, total);
    p.message = message;
    s_progress_fn(s_progress_ctx, &p);
}

void installer_progress_state(installer_state_t state, const char *message)
{
    installer_progress_step(state, 0u, 0u, message);
}

/* ---- Pre-flight decision ---------------------------------------------- */

const char *installer_error_text(installer_error_t error)
{
    switch (error) {
    case INSTALLER_ERR_NONE:            return "no error";
    case INSTALLER_ERR_DISABLED:        return "NAND write path disabled at build";
    case INSTALLER_ERR_NO_SD:           return "no SD card for backup";
    case INSTALLER_ERR_BAD_TOKEN:       return "installer package not authorized";
    case INSTALLER_ERR_BAD_LAYOUT:      return "NAND layout not recognized";
    case INSTALLER_ERR_ENCRYPTED_NAND:  return "retail encrypted NAND (AES/TMD required)";
    case INSTALLER_ERR_PROTECTED:       return "target overlaps protected boot";
    case INSTALLER_ERR_TARGET_RANGE:    return "target outside the FAT volume";
    case INSTALLER_ERR_BACKUP_FAILED:   return "NAND backup failed";
    case INSTALLER_ERR_BACKUP_MISMATCH: return "NAND backup checksum mismatch";
    case INSTALLER_ERR_UNLOCK_FAILED:   return "NAND write unlock refused";
    case INSTALLER_ERR_WRITE_FAILED:    return "NAND write failed";
    case INSTALLER_ERR_VERIFY_MISMATCH: return "read-back verify mismatch";
    default:                            return "unknown installer error";
    }
}

installer_decision_t installer_preflight(const installer_request_t *req)
{
    installer_decision_t d;

    d.proceed = false;
    d.next_state = INSTALLER_STATE_ABORTED;
    d.error = INSTALLER_ERR_NONE;

    if (req == NULL) {
        d.error = INSTALLER_ERR_DISABLED;
        return d;
    }

    /* Mandatory compile-time gate first: never even consider a write path that
     * was not explicitly armed at build time. */
    if (!req->write_enabled) {
        d.error = INSTALLER_ERR_DISABLED;
        return d;
    }
    if (!req->sd_present) {
        d.error = INSTALLER_ERR_NO_SD;
        return d;
    }
    /*
     * A retail, per-console AES-CTR encrypted image is refused before the
     * layout/token checks unless the opt-in retail write path is armed (loud
     * compile flag + retail token + runtime confirmation). Raw plaintext
     * staging can never produce a valid retail install, so this is a
     * last-resort diagnostic path only.
     */
    if (req->retail_encrypted && !req->retail_write_armed) {
        d.error = INSTALLER_ERR_ENCRYPTED_NAND;
        return d;
    }
    /* An armed retail write cannot use the (unreadable) MBR layout; the target
     * is validated against the hardcoded retail boot guard instead. */
    if (!req->layout_ok && !req->retail_write_armed) {
        d.error = INSTALLER_ERR_BAD_LAYOUT;
        return d;
    }
    if (!req->token_valid) {
        d.error = INSTALLER_ERR_BAD_TOKEN;
        return d;
    }

    d.proceed = true;
    d.next_state = req->backup_verified ? INSTALLER_STATE_UNLOCK
                                        : INSTALLER_STATE_BACKUP;
    return d;
}

/* ---- Install plan ----------------------------------------------------- */

installer_error_t installer_plan_region(const installer_token_t *token,
                                        const nand_layout_t *layout,
                                        installer_region_t *out)
{
    installer_token_status_t ts;

    if (out != NULL) {
        out->lba = 0u;
        out->count = 0u;
    }
    if (token == NULL || layout == NULL || out == NULL) {
        return INSTALLER_ERR_BAD_TOKEN;
    }

    ts = installer_token_check_range(token, layout);
    if (ts == INSTALLER_TOKEN_ERR_NULL) {
        return INSTALLER_ERR_BAD_TOKEN;
    }
    if (ts != INSTALLER_TOKEN_OK) {
        return INSTALLER_ERR_TARGET_RANGE;
    }
    /* The flashed region is exactly the payload size: no slack tail whose
     * contents the digest would not cover. */
    if (token->payload_sectors != token->target_sectors) {
        return INSTALLER_ERR_TARGET_RANGE;
    }
    if (nand_region_is_protected(layout, token->target_lba,
                                 token->target_sectors)) {
        return INSTALLER_ERR_PROTECTED;
    }

    out->lba = token->target_lba;
    out->count = token->target_sectors;
    return INSTALLER_ERR_NONE;
}
