/*
 * tests/test_menu.c
 *
 * Host-side unit tests for the menu state machine and the keypad edge
 * detector (the hardware-independent parts of src/arm9/menu.c and
 * src/arm9/input.c).
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arm9/boot.h"
#include "arm9/boot_cfg.h"
#include "arm9/menu.h"
#include "arm9/input.h"

/* ------------------------------------------------------------------ */
/* Menu state machine                                                  */
/* ------------------------------------------------------------------ */

static void test_menu_state(void)
{
    menu_state_t m;

    menu_state_init(&m, MENU_ITEM_COUNT);
    assert(m.count == MENU_ITEM_COUNT);
    assert(menu_state_selected(&m) == 0u);
    assert(menu_action_for(&m) == MENU_ACTION_SELFTEST);

    menu_state_move(&m, 1);
    assert(menu_state_selected(&m) == 1u);
    assert(menu_action_for(&m) == MENU_ACTION_INSPECT);

    menu_state_move(&m, -1);
    assert(menu_state_selected(&m) == 0u);

    /* Wrapping at both ends. */
    menu_state_move(&m, -1);
    assert(menu_state_selected(&m) == (u8)(MENU_ITEM_COUNT - 1u));
    assert(menu_action_for(&m) == MENU_ACTION_INSTALL);

    menu_state_move(&m, 1);
    assert(menu_state_selected(&m) == 0u);

    /* Large deltas wrap modulo the item count. */
    menu_state_move(&m, 100);
    assert(menu_state_selected(&m) == (u8)(100 % (int)MENU_ITEM_COUNT));

    menu_state_move(&m, -100);
    assert(menu_state_selected(&m) == 0u);

    /* Full action mapping. */
    m.cursor = 0u; assert(menu_action_for(&m) == MENU_ACTION_SELFTEST);
    m.cursor = 1u; assert(menu_action_for(&m) == MENU_ACTION_INSPECT);
    m.cursor = 2u; assert(menu_action_for(&m) == MENU_ACTION_SD);
    m.cursor = 3u; assert(menu_action_for(&m) == MENU_ACTION_LZ77);
    m.cursor = 4u; assert(menu_action_for(&m) == MENU_ACTION_HALT);
    m.cursor = 5u; assert(menu_action_for(&m) == MENU_ACTION_BROWSE);
    m.cursor = 6u; assert(menu_action_for(&m) == MENU_ACTION_SCFG_INIT);
    m.cursor = 7u; assert(menu_action_for(&m) == MENU_ACTION_AUTOBOOT);
    m.cursor = 8u; assert(menu_action_for(&m) == MENU_ACTION_INSPECT_CFG);
    m.cursor = 9u; assert(menu_action_for(&m) == MENU_ACTION_INSTALL);
    m.cursor = 10u; assert(menu_action_for(&m) == MENU_ACTION_NONE);

    /* Degenerate inputs must not misbehave. */
    menu_state_init(&m, 0u);
    menu_state_move(&m, 3);
    assert(menu_state_selected(&m) == 0u);

    menu_state_init(NULL, MENU_ITEM_COUNT);
    menu_state_move(NULL, 1);
    assert(menu_state_selected(NULL) == 0u);
    assert(menu_action_for(NULL) == MENU_ACTION_NONE);
}

/* ------------------------------------------------------------------ */
/* Input normalisation + edge detection                                */
/* ------------------------------------------------------------------ */

static void test_input_normalise(void)
{
    /* Active-low KEYINPUT: a clear bit means the button is held. */
    assert(input_normalize_keys(0xFFFFu) == 0u);
    assert(input_normalize_keys(0xFFFEu) == INPUT_A);
    assert(input_normalize_keys(0xFFF7u) == INPUT_START);
    assert(input_normalize_keys(0xFFBFu) == INPUT_UP);     /* bit 6 clear */
    assert(input_normalize_keys(0x0000u) == INPUT_KEY_MASK);

    /* EXTKEYIN: X/Y/pen/hinge. */
    assert(input_normalize_ext(0xFFFFu) == 0u);
    assert(input_normalize_ext(0xFFFEu) == INPUT_X);       /* bit 0 */
    assert(input_normalize_ext(0xFFFDu) == INPUT_Y);       /* bit 1 */
    assert(input_normalize_ext(0xFFBFu) == INPUT_PEN);     /* bit 6 */
    assert(input_normalize_ext(0xFF7Fu) == INPUT_HINGE);   /* bit 7 */
    /* Unrelated bits are ignored. */
    assert(input_normalize_ext(0xFF00u) ==
           (INPUT_X | INPUT_Y | INPUT_PEN | INPUT_HINGE));
    assert(input_normalize_ext(0xFF7Eu) == (INPUT_X | INPUT_HINGE));
}

static void test_input_edges(void)
{
    assert(input_edge_down(0x0001u, 0x0000u) == 0x0001u);
    assert(input_edge_down(0x0001u, 0x0001u) == 0x0000u);
    assert(input_edge_down(0x0003u, 0x0001u) == 0x0002u);

    assert(input_edge_up(0x0000u, 0x0001u) == 0x0001u);
    assert(input_edge_up(0x0001u, 0x0001u) == 0x0000u);
    assert(input_edge_up(0x0001u, 0x0003u) == 0x0002u);
}

static void test_input_state(void)
{
    input_state_t s;

    s.held = INPUT_SELECT | INPUT_DOWN;
    s.down = INPUT_DOWN;
    s.up = INPUT_A;

    assert(input_held(&s, INPUT_SELECT));
    assert(input_held(&s, INPUT_SELECT | INPUT_A));
    assert(!input_held(&s, INPUT_A));
    assert(input_pressed(&s, INPUT_DOWN));
    assert(!input_pressed(&s, INPUT_SELECT));
}

/* ------------------------------------------------------------------ */
/* Unlaunch-style boot policy                                          */
/* ------------------------------------------------------------------ */

static void test_boot_policy(void)
{
    /* No keys held: the active-low KEYINPUT word reads all ones. */
    assert(boot_resolve_policy(0xFFFFu) == BOOT_POLICY_AUTOBOOT);

    /* A held (active-low: bit 0 clear) -> probe/autoboot. */
    assert(boot_resolve_policy((u16)(0xFFFFu & ~KEY_A))
           == BOOT_POLICY_AUTOBOOT);

    /* B held -> force the interactive menu. */
    assert(boot_resolve_policy((u16)(0xFFFFu & ~KEY_B)) == BOOT_POLICY_MENU);

    /* SELECT held -> also force the menu. */
    assert(boot_resolve_policy((u16)(0xFFFFu & ~KEY_SELECT))
           == BOOT_POLICY_MENU);

    /* B + A: the menu override wins. */
    assert(boot_resolve_policy((u16)(0xFFFFu & ~(KEY_A | KEY_B)))
           == BOOT_POLICY_MENU);
}

/* ------------------------------------------------------------------ */
/* BOOT.CFG parsing + hotkey target resolution                         */
/* ------------------------------------------------------------------ */

static void test_boot_cfg_defaults(void)
{
    boot_cfg_t cfg;

    boot_cfg_defaults(&cfg);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_X], "NDS/HELLO.NDS") == 0);
    assert(cfg.target[BOOT_CFG_KEY_Y][0] == '\0');
    assert(cfg.target[BOOT_CFG_KEY_L][0] == '\0');
    assert(cfg.target[BOOT_CFG_KEY_R][0] == '\0');
    assert(cfg.target[BOOT_CFG_KEY_START][0] == '\0');

    assert(strcmp(boot_cfg_key_name(BOOT_CFG_KEY_DEFAULT), "DEFAULT") == 0);
    assert(strcmp(boot_cfg_key_name(BOOT_CFG_KEY_X), "X") == 0);
    assert(strcmp(boot_cfg_key_name(BOOT_CFG_KEY_START), "START") == 0);

    boot_cfg_defaults(NULL);   /* must not crash */
}

static void test_boot_cfg_parse(void)
{
    boot_cfg_t cfg;
    const char *lf =
        "# comment\n"
        "DEFAULT=BOOT.NDS\n"
        "A=HELLO.NDS\n"
        "X=NDS/HELLO.NDS\n";
    const char *crlf =
        "# comment\r\n"
        "DEFAULT=GAME.NDS\r\n"
        "A=MYGAME.NDS\r\n"
        "X=NDS/MYGAME.NDS\r\n"
        "Y=Y.NDS\r\n"
        "L=L.NDS\r\n"
        "R=R.SRL\r\n"
        "START=START.NDS\r\n";

    /* LF endings. */
    assert(boot_cfg_parse((const u8 *)lf, (u32)strlen(lf), &cfg));
    assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_X], "NDS/HELLO.NDS") == 0);
    assert(cfg.target[BOOT_CFG_KEY_Y][0] == '\0');

    /* CRLF endings and every supported key. */
    assert(boot_cfg_parse((const u8 *)crlf, (u32)strlen(crlf), &cfg));
    assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "GAME.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "MYGAME.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_X], "NDS/MYGAME.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_Y], "Y.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_L], "L.NDS") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_R], "R.SRL") == 0);
    assert(strcmp(cfg.target[BOOT_CFG_KEY_START], "START.NDS") == 0);

    /* A config without a trailing newline is accepted. */
    {
        const char *nonl = "DEFAULT=TAIL.NDS";
        assert(boot_cfg_parse((const u8 *)nonl, (u32)strlen(nonl), &cfg));
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "TAIL.NDS") == 0);
    }

    /* Every truncated prefix is handled without reading past the buffer. */
    {
        const char *full = "DEFAULT=TAIL.NDS\nA=HELLO.NDS\n";
        u32 flen = (u32)strlen(full);
        u32 i;

        for (i = 0u; i <= flen; i++) {
            assert(boot_cfg_parse((const u8 *)full, i, &cfg));
            /* Never longer than the longest possible target. */
            assert(strlen(cfg.target[BOOT_CFG_KEY_DEFAULT]) < BOOT_CFG_PATH_MAX);
        }
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "TAIL.NDS") == 0);
    }

    /* Unknown keys (including B and SELECT) are ignored. */
    {
        const char *unknown =
            "B=EVIL.NDS\nSELECT=EVIL.NDS\nFOO=BAR.NDS\n=EMPTY\nA\nDEFAULT=\n";
        assert(boot_cfg_parse((const u8 *)unknown, (u32)strlen(unknown),
                              &cfg));
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
        assert(cfg.target[BOOT_CFG_KEY_Y][0] == '\0');
    }

    /* Invalid characters and over-long targets are dropped. */
    {
        const char *bad =
            "A=BAD NAME.NDS\n"
            "X=THIS/IS/FAR/TOO/LONG/TO/FIT.NDS\n"
            "DEFAULT=OK.NDS\n";
        assert(boot_cfg_parse((const u8 *)bad, (u32)strlen(bad), &cfg));
        assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "HELLO.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_X], "NDS/HELLO.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "OK.NDS") == 0);
    }

    /* Whitespace and inline comments are trimmed; the last duplicate wins. */
    {
        const char *dup = "  DEFAULT = A.NDS # trailing\nDEFAULT=B.NDS\n"
                          "  A = OK.NDS  \n";
        assert(boot_cfg_parse((const u8 *)dup, (u32)strlen(dup), &cfg));
        assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "B.NDS") == 0);
        assert(strcmp(cfg.target[BOOT_CFG_KEY_A], "OK.NDS") == 0);
    }

    /* Degenerate arguments: defaults are still produced on a valid buffer. */
    assert(boot_cfg_parse(NULL, 0u, &cfg));
    assert(strcmp(cfg.target[BOOT_CFG_KEY_DEFAULT], "BOOT.NDS") == 0);
    assert(!boot_cfg_parse(NULL, 8u, &cfg));
    assert(!boot_cfg_parse((const u8 *)lf, (u32)strlen(lf), NULL));
}

static void test_boot_resolve_target(void)
{
    boot_cfg_t cfg;
    boot_target_t t;

    boot_cfg_defaults(&cfg);

    /* No keys: the DEFAULT target. */
    boot_resolve_target(0u, &cfg, &t);
    assert(t.policy == BOOT_POLICY_AUTOBOOT);
    assert(!t.has_key);
    assert(strcmp(t.target, "BOOT.NDS") == 0);

    /* A selects the A target. */
    boot_resolve_target(INPUT_A, &cfg, &t);
    assert(t.policy == BOOT_POLICY_AUTOBOOT);
    assert(t.has_key);
    assert(strcmp(t.target, "HELLO.NDS") == 0);
    assert(strcmp(t.label, "A") == 0);

    /* X maps to the one-level subdirectory default. */
    boot_resolve_target(INPUT_X, &cfg, &t);
    assert(t.has_key);
    assert(strcmp(t.target, "NDS/HELLO.NDS") == 0);
    assert(strcmp(t.label, "X") == 0);

    /* An unbound key (Y) falls back to DEFAULT with has_key clear. */
    boot_resolve_target(INPUT_Y, &cfg, &t);
    assert(!t.has_key);
    assert(strcmp(t.target, "BOOT.NDS") == 0);

    /* B and SELECT always force the menu, overriding every mapping. */
    boot_resolve_target(INPUT_B, &cfg, &t);
    assert(t.policy == BOOT_POLICY_MENU);
    boot_resolve_target(INPUT_SELECT, &cfg, &t);
    assert(t.policy == BOOT_POLICY_MENU);
    boot_resolve_target((u16)(INPUT_B | INPUT_A), &cfg, &t);
    assert(t.policy == BOOT_POLICY_MENU);

    /* A priority order: A beats X when both are held. */
    boot_resolve_target((u16)(INPUT_A | INPUT_X), &cfg, &t);
    assert(strcmp(t.target, "HELLO.NDS") == 0);
    assert(strcmp(t.label, "A") == 0);

    /* A parsed config overrides the defaults. */
    {
        const char *text = "DEFAULT=BOOT.NDS\nA=HELLO.NDS\nX=NDS/SUBAPP.NDS\n";
        assert(boot_cfg_parse((const u8 *)text, (u32)strlen(text), &cfg));
        boot_resolve_target(INPUT_X, &cfg, &t);
        assert(strcmp(t.target, "NDS/SUBAPP.NDS") == 0);
        boot_resolve_target(0u, &cfg, &t);
        assert(strcmp(t.target, "BOOT.NDS") == 0);
    }

    /* A NULL cfg selects the built-in defaults; a NULL out is a no-op. */
    boot_resolve_target(INPUT_A, NULL, &t);
    assert(strcmp(t.target, "HELLO.NDS") == 0);
    boot_resolve_target(0u, &cfg, NULL);
}

int main(void)
{
    test_menu_state();
    test_input_normalise();
    test_input_edges();
    test_input_state();
    test_boot_policy();

    test_boot_cfg_defaults();
    test_boot_cfg_parse();
    test_boot_resolve_target();

    puts("test_menu: all assertions passed");
    return 0;
}
