/*
 * include/arm9/boot_cfg.h
 *
 * Unlaunch-style hotkey target map (BOOT.CFG) and the pure startup-target
 * resolver.
 *
 * BOOT.CFG is an optional, read-only text file in the SD-card root that binds
 * the buttons held at startup to specific .nds/.srl images:
 *
 *     # OPENUNLAUNCH autoboot configuration
 *     DEFAULT=BOOT.NDS
 *     A=HELLO.NDS
 *     X=NDS/SUBAPP.NDS
 *
 * Supported keys are DEFAULT, A, X, Y, L, R and START. The parser is a pure,
 * zero-allocation, bounds-checked routine over a single 512-byte sector: it
 * ignores blank lines and `#` comments, tolerates CR/LF/CRLF line endings and
 * rejects malformed lines or invalid characters without ever reading past the
 * supplied buffer. B and SELECT are deliberately *not* recognised keys: they
 * can never be overridden and always force the interactive diagnostic menu
 * (BOOT_POLICY_MENU).
 */
#ifndef DSI_ARM9_BOOT_CFG_H
#define DSI_ARM9_BOOT_CFG_H

#include "../common/types.h"
#include "boot.h"

/* Maximum bytes read from BOOT.CFG (one sector). */
#define BOOT_CFG_READ_LEN 512u

/*
 * Longest accepted target, including its NUL. A path is "<DIR>/<NAME.EXT>"
 * for the one-level form, so this covers "NDS/SUBAPP.NDS" with head-room.
 */
#define BOOT_CFG_PATH_MAX 20u

/* The key slots, in the order they are listed/parsed. DEFAULT is the fallback
 * used when no mapped button is held. */
typedef enum {
    BOOT_CFG_KEY_DEFAULT = 0,
    BOOT_CFG_KEY_A,
    BOOT_CFG_KEY_X,
    BOOT_CFG_KEY_Y,
    BOOT_CFG_KEY_L,
    BOOT_CFG_KEY_R,
    BOOT_CFG_KEY_START,
    BOOT_CFG_KEY_COUNT
} boot_cfg_key_t;

/* The parsed hotkey table. Every target is NUL-terminated. */
typedef struct {
    char target[BOOT_CFG_KEY_COUNT][BOOT_CFG_PATH_MAX];
} boot_cfg_t;

/* Built-in defaults (used when BOOT.CFG is absent or a key is unspecified). */
#define BOOT_CFG_DEFAULT_TARGET  "BOOT.NDS"
#define BOOT_CFG_A_TARGET        "HELLO.NDS"
#define BOOT_CFG_X_TARGET        "NDS/HELLO.NDS"

/*
 * Reset `out` to the built-in defaults (DEFAULT = BOOT.NDS, A = HELLO.NDS,
 * X = NDS/HELLO.NDS; Y/L/R/START unbound). Pure.
 */
void boot_cfg_defaults(boot_cfg_t *out);

/*
 * Parse a `KEY=PATH` configuration from `buf` (`len` readable bytes) into
 * `out`.
 *
 * `out` is always first reset to the defaults, then every recognised line
 * overrides the matching key (last one wins). Blank lines, `#` comments
 * (whole-line and trailing), unknown keys (including B and SELECT) and
 * malformed lines are ignored. Invalid path characters or an over-long path
 * cause that line to be dropped. The routine is fully bounded: a buffer that
 * ends without a newline (a truncated read) is handled and never over-read.
 *
 * Returns true when `out` was populated (i.e. `out != NULL`); false only for a
 * NULL destination or a NULL buffer with a non-zero length.
 */
bool boot_cfg_parse(const u8 *buf, u32 len, boot_cfg_t *out);

/* Human-readable key label ("DEFAULT", "A", … "START"); never NULL. */
const char *boot_cfg_key_name(boot_cfg_key_t key);

/*
 * The resolved startup decision. `target` points either into the supplied
 * `boot_cfg_t` or at a static built-in default, so it is valid for as long as
 * the configuration object is.
 */
typedef struct {
    boot_policy_t policy;   /* MENU (B/SELECT held) or AUTOBOOT             */
    const char   *target;   /* resolved .nds/.srl path (NUL-terminated)     */
    const char   *label;    /* held key label ("A", "X", …) or NULL         */
    bool has_key;           /* a mapped, bound key selected the target      */
} boot_target_t;

/*
 * Resolve the startup target. `held_keys` is the active-high logical key mask
 * captured at startup — i.e. `input_state_t.held`, which is KEYINPUT
 * normalised by input_normalize_keys() and OR-ed with the ARM7-forwarded
 * EXTKEYIN bits (so X/Y are visible once input_update_ext() has run).
 *
 * B/SELECT always yield BOOT_POLICY_MENU; otherwise the held mapped button is
 * looked up (A, X, Y, L, R, START in that priority order) and, failing that,
 * the DEFAULT target is used. An unbound button falls back to DEFAULT with
 * `has_key == false`.
 *
 * A NULL `cfg` selects the static built-in defaults. Pure and host-testable.
 */
void boot_resolve_target(u16 held_keys, const boot_cfg_t *cfg,
                         boot_target_t *out);

#endif /* DSI_ARM9_BOOT_CFG_H */
