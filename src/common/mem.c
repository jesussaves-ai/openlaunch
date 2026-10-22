/*
 * src/common/mem.c
 *
 * Freestanding memory primitives — see include/common/mem.h for the reference
 * symbol mapping.
 */
#include "common/mem.h"

/*
 * Fill `len` bytes of `dst` with `value`.
 *
 * @ref FUN_0001c72c  (address 0x01C72C)
 *   r0 = destination, r1 = byte value, r2 = length.
 *   The reference aligns to 2/4 bytes and uses a 10-word unrolled loop; here we
 *   keep the same alignment strategy but express it readably.
 */
void *dsi_memset(void *dst, int value, size_t len)
{
    u8 *p = (u8 *)dst;
    const u8 byte = (u8)value;

    /* Byte head until 4-byte aligned. */
    while (len != 0u && (((uintptr_t)p & 3u) != 0u)) {
        *p++ = byte;
        len--;
    }

    if (len >= 4u) {
        u32 word = (u32)byte;
        word |= word << 8;
        word |= word << 16;

        while (len >= 4u) {
            *(vu32 *)(void *)p = word;
            p += 4;
            len -= 4u;
        }
    }

    /* Byte tail. */
    while (len != 0u) {
        *p++ = byte;
        len--;
    }
    return dst;
}

/*
 * Copy `len` bytes from `src` to `dst`. Overlap is not supported (matches the
 * reference memcpy contract).
 *
 * @ref FUN_0001c7d0  (address 0x01C7D0)
 *   r0 = destination, r1 = source, r2 = length.
 */
void *dsi_memcpy(void *dst, const void *src, size_t len)
{
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    if (len == 0u) {
        return dst;
    }

    /* Align both pointers together (1 byte at a time). */
    while (len != 0u &&
           ((((uintptr_t)d ^ (uintptr_t)s) & 1u) != 0u)) {
        *d++ = *s++;
        len--;
    }
    while (len >= 2u &&
           ((((uintptr_t)s) & 3u) != 0u)) {
        *(vu16 *)(void *)d = *(const vu16 *)(const void *)s;
        d += 2;
        s += 2;
        len -= 2u;
    }
    while (len >= 4u && (((uintptr_t)s & 3u) == 0u)) {
        *(vu32 *)(void *)d = *(const vu32 *)(const void *)s;
        d += 4;
        s += 4;
        len -= 4u;
    }
    while (len != 0u) {
        *d++ = *s++;
        len--;
    }
    return dst;
}

/*
 * Lexicographic byte comparison.
 *
 * @ref FUN_0001caa8  (address 0x01CAA8)
 */
int dsi_memcmp(const void *a, const void *b, size_t len)
{
    const u8 *pa = (const u8 *)a;
    const u8 *pb = (const u8 *)b;

    while (len != 0u) {
        u8 va = *pa++;
        u8 vb = *pb++;
        if (va != vb) {
            return (int)va - (int)vb;
        }
        len--;
    }
    return 0;
}

/*
 * Length of a NUL-terminated string.
 *
 * @ref FUN_0001ca50  (address 0x01CA50)
 */
size_t dsi_strlen(const char *s)
{
    const char *p = s;
    while (*p != '\0') {
        p++;
    }
    return (size_t)(p - s);
}

/*
 * Load a 32-bit little-endian word from a possibly unaligned address.
 * Reconstructed intent of FUN_0001c8d8 (the reference's fused
 * "load word + return next pointer" pair).
 */
u32 dsi_read_le32(const void *p)
{
    const u8 *b = (const u8 *)p;
    return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
}

/* @ref FUN_0001c918 / FUN_0001c8fc — 16-bit unaligned load. */
u16 dsi_read_le16(const void *p)
{
    const u8 *b = (const u8 *)p;
    return (u16)((u16)b[0] | ((u16)b[1] << 8));
}

u8 dsi_read_u8(const void *p)
{
    return *(const u8 *)p;
}

/* @ref FUN_0001c92c — unaligned little-endian 32-bit store. */
void dsi_write_le32(void *p, u32 value)
{
    u8 *b = (u8 *)p;
    b[0] = (u8)(value & 0xFFu);
    b[1] = (u8)((value >> 8) & 0xFFu);
    b[2] = (u8)((value >> 16) & 0xFFu);
    b[3] = (u8)((value >> 24) & 0xFFu);
}
