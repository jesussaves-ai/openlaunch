/*
 * include/common/installer.h
 *
 * Pure, device-independent policy for the OpenLaunch NAND installer:
 *
 *   - the signed installer-package token layout, its HMAC-SHA-256
 *     authentication and its range checks;
 *   - the installer state machine (backup -> verify -> unlock -> write ->
 *     verify) and its abort/error vocabulary;
 *   - the pre-flight decision that refuses to proceed unless *every* safety
 *     condition holds (write path compiled in, SD backup destination present,
 *     authenticated token, validated layout, in-range non-protected target).
 *
 * Nothing here touches hardware. The ARM9 glue that performs the transfers
 * lives in src/arm9/installer_backup.c and src/arm9/installer_install.c, and
 * the whole write path is compiled only under ENABLE_NAND_WRITE_DANGER
 * (the project conventions). Keeping the policy pure makes the safety rules host-testable.
 */
#ifndef DSI_COMMON_INSTALLER_H
#define DSI_COMMON_INSTALLER_H

#include "types.h"
#include "sha256.h"
#include "nand.h"

/* ---- Signed installer token ------------------------------------------- */

#define INSTALLER_TOKEN_MAGIC   0x4F4C4E43u   /* "OLNC" (little-endian)     */
#define INSTALLER_TOKEN_VERSION 1u

/*
 * Token flag bit 0: the package targets a decrypted sandbox image (an emulator
 * or developer staging volume), not a retail console. It lets the installer
 * bound the mandatory backup to a size that fits a small emulator SD image
 * (see INSTALLER_SANDBOX_BACKUP_SECTORS in include/arm9/installer.h). It is
 * signed like every other field and can never enable writing on a retail image,
 * which the encrypted-NAND guard refuses first.
 */
#define INSTALLER_TOKEN_FLAG_SANDBOX 0x0001u

/*
 * Token flag bit 1: the package authorises a raw write to a retail console.
 * Only honoured when the bootloader is also compiled with the loud
 * ENABLE_RETAIL_NAND_WRITE flag (Makefile: `make installer-retail`), the
 * runtime confirmation is held, and the target lies in the hardcoded retail
 * boot-safe window. See the safety note in docs/hardware_notes.md 34.11.
 */
#define INSTALLER_TOKEN_FLAG_RETAIL 0x0002u

/*
 * On-disk token layout (96 bytes, little-endian):
 *
 *   0x00  u32  magic
 *   0x04  u16  version
 *   0x06  u16  flags
 *   0x08  u32  target_lba        first NAND sector to overwrite
 *   0x0C  u32  target_sectors    number of sectors to overwrite
 *   0x10  u32  payload_sectors   OpenLaunch payload size in sectors
 *   0x14  u8[32] payload_sha256  digest of the payload image
 *   0x34  u8[12] reserved        must be zero
 *   0x40  u8[32] signature       HMAC-SHA-256 of bytes [0, 0x40)
 *
 * The HMAC key is compiled in (see INSTALLER_AUTH_KEY in installer.c) and is
 * intentionally *not* a substitute for a public-key scheme: it is a
 * tamper-evident gate layered on top of the mandatory ENABLE_NAND_WRITE_DANGER
 * build flag, not a replacement for it.
 */
#define INSTALLER_TOKEN_SIZE      96u
#define INSTALLER_TOKEN_SIGNED_LEN 0x40u

typedef struct {
    u32 magic;
    u16 version;
    u16 flags;
    u32 target_lba;
    u32 target_sectors;
    u32 payload_sectors;
    u8  payload_sha256[SHA256_DIGEST_SIZE];
    u8  signature[SHA256_DIGEST_SIZE];
} installer_token_t;

typedef enum {
    INSTALLER_TOKEN_OK = 0,
    INSTALLER_TOKEN_ERR_NULL,
    INSTALLER_TOKEN_ERR_SIZE,
    INSTALLER_TOKEN_ERR_MAGIC,
    INSTALLER_TOKEN_ERR_VERSION,
    INSTALLER_TOKEN_ERR_SIGNATURE,
    INSTALLER_TOKEN_ERR_RANGE
} installer_token_status_t;

/*
 * Decode and structurally validate a token from `len` bytes at `buf`. The
 * signature is checked by installer_token_verify() so a caller can inspect the
 * advertised target before deciding to authenticate it. Purely parses; no key.
 */
installer_token_status_t installer_token_parse(const u8 *buf, u32 len,
                                               installer_token_t *out);

/* Recompute the HMAC-SHA-256 over the signed prefix and compare in constant
 * time. Returns INSTALLER_TOKEN_OK only on an exact match. */
installer_token_status_t installer_token_verify(const installer_token_t *token,
                                                const u8 *key, u32 key_len);

/* Serialise + sign a token into a 96-byte buffer (used by the package tooling
 * and the tests). Reserved bytes are zeroed. */
void installer_token_encode(const installer_token_t *token,
                            const u8 *key, u32 key_len,
                            u8 out[INSTALLER_TOKEN_SIZE]);

/*
 * Structurally validate the target against a decoded volume geometry: the
 * target must be non-empty, inside the volume and outside the protected boot
 * prefix. Used by the pre-flight decision (see installer_plan_region()).
 */
installer_token_status_t installer_token_check_range(const installer_token_t *t,
                                                     const nand_layout_t *layout);

const char *installer_token_status_text(installer_token_status_t status);

/* ---- Installer state machine ------------------------------------------ */

typedef enum {
    INSTALLER_STATE_IDLE = 0,
    INSTALLER_STATE_PREFLIGHT,
    INSTALLER_STATE_BACKUP,
    INSTALLER_STATE_VERIFY_BACKUP,
    INSTALLER_STATE_UNLOCK,
    INSTALLER_STATE_WRITE,
    INSTALLER_STATE_VERIFY_WRITE,
    INSTALLER_STATE_DONE,
    INSTALLER_STATE_ABORTED
} installer_state_t;

const char *installer_state_text(installer_state_t state);

/*
 * Pure transition: given the current state and whether the step succeeded,
 * return the next state. Any failure moves to INSTALLER_STATE_ABORTED.
 */
installer_state_t installer_advance(installer_state_t state, bool step_ok);

/* ---- Progress observation --------------------------------------------- */

/*
 * Live progress snapshot handed to the optional hook installed with
 * installer_set_progress_hook(). The hook is purely observational: the
 * installer never consults its return value or its contents, so the host tests
 * (which leave it unset) and the on-screen modal progress renderer share
 * exactly the same control flow and results.
 */
typedef struct {
    installer_state_t state;   /* the state currently executing            */
    u32               done;    /* units (512-byte sectors) done in this step*/
    u32               total;   /* units in this step (0 = not a stream)     */
    u8                percent; /* 0..100, derived from done/total           */
    const char       *message; /* short, static status text                 */
} installer_progress_t;

typedef void (*installer_progress_fn)(void *ctx,
                                      const installer_progress_t *progress);

/*
 * Install (or clear, with a NULL `fn`) the single global progress hook. The
 * firmware's menu installs its modal renderer here; a NULL hook makes every
 * report a no-op, which is how the host suites run.
 */
void installer_set_progress_hook(installer_progress_fn fn, void *ctx);

/* Pure: the integer percentage of `done`/`total`, clamped to 0..100. */
u8 installer_percent(u32 done, u32 total);

/* Report a state transition that carries no per-sector count. No-op without a
 * hook; safe to call with a NULL message. */
void installer_progress_state(installer_state_t state, const char *message);

/* Report progress inside a counted stream (backup/write). No-op without a
 * hook. `done` is clamped to `total` for the percentage. */
void installer_progress_step(installer_state_t state, u32 done, u32 total,
                             const char *message);

/* ---- Pre-flight decision ---------------------------------------------- */

typedef enum {
    INSTALLER_ERR_NONE = 0,
    INSTALLER_ERR_DISABLED,        /* built without ENABLE_NAND_WRITE_DANGER */
    INSTALLER_ERR_NO_SD,           /* backup destination unavailable        */
    INSTALLER_ERR_BAD_TOKEN,       /* missing / unsigned / out-of-range      */
    INSTALLER_ERR_BAD_LAYOUT,      /* LBA 0 was not a usable NAND MBR        */
    INSTALLER_ERR_ENCRYPTED_NAND,  /* retail AES-CTR image: raw staging unsafe */
    INSTALLER_ERR_PROTECTED,       /* target overlaps the boot prefix        */
    INSTALLER_ERR_TARGET_RANGE,    /* target outside the FAT partition       */
    INSTALLER_ERR_BACKUP_FAILED,
    INSTALLER_ERR_BACKUP_MISMATCH,
    INSTALLER_ERR_UNLOCK_FAILED,
    INSTALLER_ERR_WRITE_FAILED,
    INSTALLER_ERR_VERIFY_MISMATCH
} installer_error_t;

const char *installer_error_text(installer_error_t error);

/* Snapshot of the conditions the pre-flight gate evaluates. */
typedef struct {
    bool write_enabled;     /* ENABLE_NAND_WRITE_DANGER compiled in       */
    bool sd_present;        /* the SD backup destination is mounted       */
    bool retail_encrypted;  /* the eMMC image is retail AES-CTR encrypted  */
    bool retail_write_armed;/* ENABLE_RETAIL_NAND_WRITE + a retail token   */
    bool token_valid;       /* token authenticated AND target in range    */
    bool layout_ok;         /* nand_validate_layout() succeeded           */
    bool backup_verified;   /* the full NAND dump was written + hashed    */
} installer_request_t;

typedef struct {
    bool proceed;                 /* true => enter INSTALLER_STATE_BACKUP */
    installer_state_t next_state; /* BACKUP or ABORTED                    */
    installer_error_t error;      /* INSTALLER_ERR_NONE when proceed       */
} installer_decision_t;

/*
 * Evaluate the gate in strict order: build flag, SD destination, retail
 * encryption, layout, token, and finally the previously completed backup.
 * Returns the decision and the error that vetoed it (INSTALLER_ERR_NONE when it
 * may proceed).
 */
installer_decision_t installer_preflight(const installer_request_t *req);

/* ---- Install plan ----------------------------------------------------- */

typedef struct {
    u32 lba;      /* first target sector */
    u32 count;    /* number of sectors   */
} installer_region_t;

/*
 * Resolve the (already authenticated) token into a concrete region inside the
 * validated layout. Fails with INSTALLER_ERR_PROTECTED or
 * INSTALLER_ERR_TARGET_RANGE when the region is unsafe. Pure.
 */
installer_error_t installer_plan_region(const installer_token_t *token,
                                        const nand_layout_t *layout,
                                        installer_region_t *out);

#endif /* DSI_COMMON_INSTALLER_H */
