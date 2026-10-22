/*
 * src/arm9/input.c
 *
 * Keypad polling and edge detection. See include/arm9/input.h.
 *
 * KEYINPUT (0x04000130) is a 16-bit active-low register; bits 0-9 map to
 * A/B/Select/Start/Right/Left/Up/Down/R/L. EXTKEYIN (0x04000136) is ARM7-only,
 * so its bits arrive through input_update_ext().
 */
#include "arm9/input.h"
#include "common/dsi_hw.h"

/* Cached, normalised extension keys (X/Y/pen/hinge) from the ARM7. */
static u16 s_ext_pressed;

void input_init(void)
{
    s_ext_pressed = 0u;
}

u16 input_normalize_keys(u16 raw_keyinput)
{
    /* Active-low: a zero bit means the button is held. */
    return (u16)(~raw_keyinput) & INPUT_KEY_MASK;
}

u16 input_normalize_ext(u16 raw_extkeyin)
{
    u16 pressed = (u16)(~raw_extkeyin);
    u16 out = 0u;

    if ((pressed & EXTKEY_X) != 0u) {
        out |= INPUT_X;
    }
    if ((pressed & EXTKEY_Y) != 0u) {
        out |= INPUT_Y;
    }
    if ((pressed & EXTKEY_PEN) != 0u) {
        out |= INPUT_PEN;
    }
    if ((pressed & EXTKEY_HINGE) != 0u) {
        out |= INPUT_HINGE;
    }
    return out;
}

u16 input_edge_down(u16 held, u16 prev)
{
    return (u16)(held & (u16)~prev);
}

u16 input_edge_up(u16 held, u16 prev)
{
    return (u16)((u16)~held & prev);
}

void input_update_ext(u16 ext_pressed)
{
    s_ext_pressed = ext_pressed & INPUT_EXT_MASK;
}

void input_poll(input_state_t *state)
{
    u16 prev;
    u16 held;

    if (state == NULL) {
        return;
    }

    prev = state->held;
    held = input_normalize_keys(REG16(KEYINPUT)) | s_ext_pressed;

    state->held = held;
    state->down = input_edge_down(held, prev);
    state->up = input_edge_up(held, prev);
}
