/*
 * include/arm9/input.h
 *
 * Keypad input for the ARM9 boot menu.
 *
 * The ARM9 reads KEYINPUT (0x04000130, active-low) directly. The extension
 * keys (X, Y, stylus/pen and hinge) live in the ARM7-only EXTKEYIN register
 * (0x04000136); the ARM7 forwards them over IPC and the menu feeds them in
 * with input_update_ext().
 *
 * Edge detection is per poll: `down` is the set of keys that went from
 * released to pressed since the previous poll, `up` the opposite.
 *
 * The normalisation and edge helpers are pure and host-testable.
 */
#ifndef DSI_ARM9_INPUT_H
#define DSI_ARM9_INPUT_H

#include "../common/types.h"

/* Logical (active-high) key bits, decoupled from the hardware layout. */
#define INPUT_A       BIT(0)
#define INPUT_B       BIT(1)
#define INPUT_SELECT  BIT(2)
#define INPUT_START   BIT(3)
#define INPUT_RIGHT   BIT(4)
#define INPUT_LEFT    BIT(5)
#define INPUT_UP      BIT(6)
#define INPUT_DOWN    BIT(7)
#define INPUT_R       BIT(8)
#define INPUT_L       BIT(9)
#define INPUT_X       BIT(10)
#define INPUT_Y       BIT(11)
#define INPUT_PEN     BIT(12)
#define INPUT_HINGE   BIT(13)

/* Bits sourced from KEYINPUT (ARM9-local). */
#define INPUT_KEY_MASK  0x03FFu
/* Bits sourced from EXTKEYIN (ARM7, forwarded over IPC). */
#define INPUT_EXT_MASK  0x3C00u

typedef struct {
    u16 held;   /* keys currently pressed                      */
    u16 down;   /* keys pressed since the previous poll        */
    u16 up;     /* keys released since the previous poll       */
} input_state_t;

/* Reset the cached extension keys and the edge state. */
void input_init(void);

/*
 * Read KEYINPUT plus the cached extension keys, then update the edge sets in
 * `state` (the previous `held` value is kept inside `state`).
 */
void input_poll(input_state_t *state);

/* Feed the ARM7-forwarded EXTKEYIN bits (already normalised, active-high). */
void input_update_ext(u16 ext_pressed);

/* ---- Pure helpers (host-testable) ------------------------------------ */

/* Convert a raw active-low KEYINPUT word into active-high pressed bits. */
u16 input_normalize_keys(u16 raw_keyinput);

/* Convert raw active-low EXTKEYIN bits into the logical INPUT_X/Y/PEN/HINGE. */
u16 input_normalize_ext(u16 raw_extkeyin);

/* Newly pressed / released keys given the current and previous held sets. */
u16 input_edge_down(u16 held, u16 prev);
u16 input_edge_up(u16 held, u16 prev);

/* True when any key in `mask` is held. */
static inline bool input_held(const input_state_t *s, u16 mask)
{
    return (s->held & mask) != 0u;
}

/* True when any key in `mask` went down this poll. */
static inline bool input_pressed(const input_state_t *s, u16 mask)
{
    return (s->down & mask) != 0u;
}

#endif /* DSI_ARM9_INPUT_H */
