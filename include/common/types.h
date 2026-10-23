/*
 * include/common/types.h
 *
 * Fixed-width integer and volatile-register type vocabulary shared by the
 * ARM9 and ARM7 halves of the bootloader.
 *
 * Rules (the project conventions, §3):
 *   - Strict C99, freestanding. Only <stdint.h> / <stdbool.h> / <stddef.h>.
 *   - Every hardware register access must go through a volatile-qualified
 *     lvalue (see REG8/REG16/REG32 below).
 */
#ifndef DSI_COMMON_TYPES_H
#define DSI_COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Short aliases used throughout the reverse-engineered code ---- */
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

/* Volatile aliases: never optimised away when talking to hardware. */
typedef volatile uint8_t   vu8;
typedef volatile uint16_t  vu16;
typedef volatile uint32_t  vu32;

/*
 * Memory-mapped register accessors.
 *
 * the project conventions: hardware I/O must be touched through volatile pointers so the
 * compiler cannot reorder or delete hardware handshakes. These macros expand
 * to a volatile lvalue, so `REG32(SCFG_EXT) |= 1u;` performs a read-modify-write
 * on the actual bus.
 */
#define REG8(addr)   (*(vu8  *)(uintptr_t)(addr))
#define REG16(addr)  (*(vu16 *)(uintptr_t)(addr))
#define REG32(addr)  (*(vu32 *)(uintptr_t)(addr))

/* Bit helpers (C99-safe, no GNU extensions). */
#define BIT(n)       (1UL << (n))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*
 * Compile-time assertion. C99 has no _Static_assert; use the negative-array
 * trick so misuse fails the build rather than silently corrupting a register.
 */
#define DSI_STATIC_ASSERT(cond, msg) \
    typedef char dsi_static_assert_##msg[(cond) ? 1 : -1]

#endif /* DSI_COMMON_TYPES_H */
