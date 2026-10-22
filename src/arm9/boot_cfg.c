/*
 * src/arm9/boot_cfg.c
 *
 * Unlaunch-style BOOT.CFG parser and startup-target resolver.
 * See include/arm9/boot_cfg.h for the contract.
 *
 * The parser is a single bounded pass over a 512-byte sector buffer: no
 * allocation, no global mutable state, no unbounded loops. It reconstructs the
 * intent of a simple "KEY=PATH" text file rather than mirroring any particular
 * reference code, and it is deliberately conservative (unknown keys and
 * malformed lines are ignored, never fatal).
 */
#include "arm9/boot_cfg.h"
#include "arm9/input.h"
#include "arm9/theme.h"
#include "common/mem.h"

/* The key labels, indexed by boot_cfg_key_t. */
static const char *const s_key_names[BOOT_CFG_KEY_COUNT] = {
    "DEFAULT", "A", "X", "Y", "L", "R", "START"
};

/* Optional boot-theme directives, indexed by boot_cfg_theme_key_t. */
typedef enum {
    BOOT_CFG_THEME_ACCENT = 0,
    BOOT_CFG_THEME_TEXT,
    BOOT_CFG_THEME_BG,
    BOOT_CFG_THEME_SPLASH
} boot_cfg_theme_key_t;

static const char *const s_theme_key_names[] = {
    "THEME_ACCENT", "THEME_TEXT", "THEME_BG", "SPLASH_IMAGE"
};

/* Built-in defaults, used when BOOT.CFG is absent or a key is unspecified. */
static const boot_cfg_t s_default_cfg = {
    {
        BOOT_CFG_DEFAULT_TARGET,
        BOOT_CFG_A_TARGET,
        BOOT_CFG_X_TARGET,
        "",
        "",
        "",
        ""
    },
    {
        THEME_ACCENT_BGR555_DEFAULT,
        THEME_TEXT_BGR555_DEFAULT,
        THEME_BG_BGR555_DEFAULT,
        false,
        ""
    }
};

static void copy_cstr(char *dst, const char *src)
{
    u32 i = 0u;

    while (src[i] != '\0' && i < BOOT_CFG_PATH_MAX - 1u) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void boot_cfg_defaults(boot_cfg_t *out)
{
    if (out == NULL) {
        return;
    }
    dsi_memset(out, 0, sizeof(*out));
    copy_cstr(out->target[BOOT_CFG_KEY_DEFAULT], BOOT_CFG_DEFAULT_TARGET);
    copy_cstr(out->target[BOOT_CFG_KEY_A], BOOT_CFG_A_TARGET);
    copy_cstr(out->target[BOOT_CFG_KEY_X], BOOT_CFG_X_TARGET);
    theme_defaults(&out->theme);
}

const char *boot_cfg_key_name(boot_cfg_key_t key)
{
    if ((u32)key >= BOOT_CFG_KEY_COUNT) {
        return "?";
    }
    return s_key_names[key];
}

/* ASCII case-insensitive compare of a counted token against a C string. */
static bool token_equals_ci(const u8 *p, u32 n, const char *tok)
{
    u32 i;

    for (i = 0u; i < n; i++) {
        char c = (char)p[i];
        char t = tok[i];

        if (t == '\0') {
            return false;
        }
        if (c >= 'a' && c <= 'z') { c = (char)(c - ('a' - 'A')); }
        if (t >= 'a' && t <= 'z') { t = (char)(t - ('a' - 'A')); }
        if (c != t) {
            return false;
        }
    }
    return tok[n] == '\0';
}

/* Map a key token to its slot, or -1 for an unknown / reserved key. */
static int key_from_token(const u8 *p, u32 n)
{
    u32 k;

    if (n == 0u) {
        return -1;
    }
    for (k = 0u; k < BOOT_CFG_KEY_COUNT; k++) {
        if (token_equals_ci(p, n, s_key_names[k])) {
            return (int)k;
        }
    }
    return -1;   /* includes B and SELECT, which can never be overridden */
}

/* Map a token to a theme directive, or -1 for an unknown key. */
static int theme_key_from_token(const u8 *p, u32 n)
{
    u32 k;

    if (n == 0u) {
        return -1;
    }
    for (k = 0u; k < ARRAY_SIZE(s_theme_key_names); k++) {
        if (token_equals_ci(p, n, s_theme_key_names[k])) {
            return (int)k;
        }
    }
    return -1;
}

/* Characters permitted in a target path (a conservative ASCII subset). */
static bool path_char_ok(u8 c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '.' || c == '_' || c == '-' || c == '/';
}

/*
 * Parse one logical line (already stripped of its terminator). A NUL byte ends
 * the content; a '#' at the start of a line or after whitespace starts a
 * comment (so a `#RRGGBB` colour is preserved).
 */
static void boot_cfg_line(const u8 *line, u32 n, boot_cfg_t *out)
{
    u32 i;
    u32 ks;
    u32 ke;
    u32 eq = 0u;
    bool has_eq = false;
    int keyidx;
    int themekey;

    /*
     * A NUL byte always ends the meaningful content. `#` starts a comment only
     * at the start of a line or after whitespace, so a `#RRGGBB` colour value is
     * not swallowed as a comment.
     */
    for (i = 0u; i < n; i++) {
        if (line[i] == 0x00u) {
            n = i;
            break;
        }
        if (line[i] == '#' &&
            (i == 0u || line[i - 1u] == ' ' || line[i - 1u] == '\t')) {
            n = i;
            break;
        }
    }

    ks = 0u;
    while (ks < n && (line[ks] == ' ' || line[ks] == '\t')) {
        ks++;
    }
    ke = n;
    while (ke > ks && (line[ke - 1u] == ' ' || line[ke - 1u] == '\t')) {
        ke--;
    }
    if (ks == ke) {
        return;   /* blank line / comment */
    }

    for (i = ks; i < ke; i++) {
        if (line[i] == '=') {
            has_eq = true;
            eq = i;
            break;
        }
    }
    if (!has_eq) {
        return;   /* malformed: no separator */
    }

    {
        u32 key_start = ks;
        u32 key_end = eq;
        u32 vs;
        u32 ve;

        while (key_end > key_start &&
               (line[key_end - 1u] == ' ' || line[key_end - 1u] == '\t')) {
            key_end--;
        }
        vs = eq + 1u;
        while (vs < ke && (line[vs] == ' ' || line[vs] == '\t')) {
            vs++;
        }
        ve = ke;
        while (ve > vs &&
               (line[ve - 1u] == ' ' || line[ve - 1u] == '\t')) {
            ve--;
        }

        if (key_end == key_start || ve == vs) {
            return;   /* empty key or empty value */
        }

        keyidx = key_from_token(line + key_start, key_end - key_start);
        if (keyidx >= 0) {
            if (ve - vs >= BOOT_CFG_PATH_MAX) {
                return;   /* target too long: drop the line, keep the default */
            }
            for (i = vs; i < ve; i++) {
                if (!path_char_ok(line[i])) {
                    return;   /* invalid character */
                }
            }
            dsi_memcpy(out->target[keyidx], line + vs, ve - vs);
            out->target[keyidx][ve - vs] = '\0';
            return;
        }

        /* Optional theme directives; unknown keys stay ignored. */
        themekey = theme_key_from_token(line + key_start, key_end - key_start);
        if (themekey < 0) {
            return;
        }

        switch ((boot_cfg_theme_key_t)themekey) {
        case BOOT_CFG_THEME_ACCENT:
        case BOOT_CFG_THEME_TEXT:
        case BOOT_CFG_THEME_BG: {
            u16 color;

            if (!theme_parse_color(line + vs, ve - vs, &color)) {
                return;   /* malformed colour: keep the default */
            }
            if (themekey == BOOT_CFG_THEME_ACCENT) {
                out->theme.accent = color;
            } else if (themekey == BOOT_CFG_THEME_TEXT) {
                out->theme.text = color;
            } else {
                out->theme.bg = color;
            }
            break;
        }
        case BOOT_CFG_THEME_SPLASH:
            if (ve - vs >= THEME_PATH_MAX) {
                return;   /* splash path too long */
            }
            for (i = vs; i < ve; i++) {
                if (!path_char_ok(line[i])) {
                    return;   /* invalid character */
                }
            }
            dsi_memcpy(out->theme.splash_path, line + vs, ve - vs);
            out->theme.splash_path[ve - vs] = '\0';
            out->theme.has_splash = true;
            break;
        default:
            break;
        }
    }
}

bool boot_cfg_parse(const u8 *buf, u32 len, boot_cfg_t *out)
{
    u32 pos;

    if (out == NULL) {
        return false;
    }
    boot_cfg_defaults(out);
    if (buf == NULL) {
        return len == 0u;
    }

    pos = 0u;
    while (pos < len) {
        u32 line_len = 0u;
        u32 adv;

        while (pos + line_len < len && buf[pos + line_len] != '\n' &&
               buf[pos + line_len] != '\r') {
            line_len++;
        }

        boot_cfg_line(buf + pos, line_len, out);

        adv = line_len;
        if (pos + adv < len && buf[pos + adv] == '\r') {
            adv++;
        }
        if (pos + adv < len && buf[pos + adv] == '\n') {
            adv++;
        }
        if (adv == 0u) {
            adv = 1u;   /* defensive: always make forward progress */
        }
        pos += adv;
    }
    return true;
}

/*
 * The held buttons that map to a target, in resolution priority order. B and
 * SELECT are intentionally absent: they are handled before this table and can
 * never be bound by BOOT.CFG.
 */
static const u16 s_hotkeys[6] = {
    (u16)INPUT_A, (u16)INPUT_X, (u16)INPUT_Y,
    (u16)INPUT_L, (u16)INPUT_R, (u16)INPUT_START
};

static const boot_cfg_key_t s_hotkey_slots[6] = {
    BOOT_CFG_KEY_A, BOOT_CFG_KEY_X, BOOT_CFG_KEY_Y,
    BOOT_CFG_KEY_L, BOOT_CFG_KEY_R, BOOT_CFG_KEY_START
};

void boot_resolve_target(u16 held_keys, const boot_cfg_t *cfg,
                         boot_target_t *out)
{
    const boot_cfg_t *c = (cfg != NULL) ? cfg : &s_default_cfg;
    u32 i;

    if (out == NULL) {
        return;
    }

    out->policy = BOOT_POLICY_AUTOBOOT;
    out->target = c->target[BOOT_CFG_KEY_DEFAULT];
    out->label = NULL;
    out->has_key = false;

    /* The unbrickable override: B or SELECT always forces the menu. */
    if ((held_keys & (u16)(INPUT_B | INPUT_SELECT)) != 0u) {
        out->policy = BOOT_POLICY_MENU;
        return;
    }

    for (i = 0u; i < 6u; i++) {
        if ((held_keys & s_hotkeys[i]) != 0u) {
            const char *t = c->target[s_hotkey_slots[i]];

            if (t[0] != '\0') {
                out->target = t;
                out->label = boot_cfg_key_name(s_hotkey_slots[i]);
                out->has_key = true;
            }
            return;   /* the highest-priority held button wins */
        }
    }

    if (out->target[0] == '\0') {
        out->target = BOOT_CFG_DEFAULT_TARGET;
    }
}
