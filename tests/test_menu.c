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

#include "arm9/boot.h"
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
    assert(menu_action_for(&m) == MENU_ACTION_AUTOBOOT);

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
    m.cursor = 8u; assert(menu_action_for(&m) == MENU_ACTION_NONE);

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

int main(void)
{
    test_menu_state();
    test_input_normalise();
    test_input_edges();
    test_input_state();
    test_boot_policy();

    puts("test_menu: all assertions passed");
    return 0;
}
