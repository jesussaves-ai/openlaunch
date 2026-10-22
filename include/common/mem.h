/*
 * include/common/mem.h
 *
 * Freestanding memory and byte-order primitives.
 *
 * Translated from the reference routines:
 *   FUN_0001c72c (memset)   FUN_0001c7d0 (memcpy)   FUN_0001ca50 (strlen)
 *   FUN_0001caa8 (memcmp)   FUN_0001c8d8/c8fc/c918 (unaligned LE loads)
 *   FUN_0001c92c (unaligned LE store)
 *
 * The reference used hand-unrolled, word-aligned loops; the reconstructed
 * intent is an aligned bulk copy with scalar head/tail handling. See AGENTS.md
 * §4.3 ("reconstruct intent, do not blindly emit compiler idioms").
 */
#ifndef DSI_COMMON_MEM_H
#define DSI_COMMON_MEM_H

#include "types.h"

void  *dsi_memset(void *dst, int value, size_t len);
void  *dsi_memcpy(void *dst, const void *src, size_t len);
int    dsi_memcmp(const void *a, const void *b, size_t len);
size_t dsi_strlen(const char *s);

/* Explicit little-endian accessors valid on unaligned addresses. */
u32 dsi_read_le32(const void *p);
u16 dsi_read_le16(const void *p);
u8  dsi_read_u8(const void *p);
void dsi_write_le32(void *p, u32 value);

#endif /* DSI_COMMON_MEM_H */
