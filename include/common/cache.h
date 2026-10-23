/*
 * include/common/cache.h
 *
 * ARM9/ARM7 cache maintenance helpers.
 *
 * @ref FUN_0001cae0  (address 0x01CAE0) — clean data cache by index over all
 * ways/sets, then data-synchronisation barrier, then invalidate the entire
 * instruction and data caches.
 *
 * the project conventions: flush before handing a buffer to DMA/ARM7; invalidate before
 * reading memory that DMA/ARM7 wrote. Never use DTCM as a DMA buffer.
 */
#ifndef DSI_COMMON_CACHE_H
#define DSI_COMMON_CACHE_H

#include "types.h"

/* Clean + invalidate the entire data cache, invalidate the I cache. */
void cache_flush_all(void);

/* Clean the entire data cache (write back, keep contents). */
void cache_clean_all(void);

/* Invalidate the entire data cache (discard contents). */
void cache_invalidate_all(void);

/*
 * Range helpers. On the ARM946E-S these fall back to a full-cache operation,
 * which is always correct (only slower). Fine for boot-time use.
 */
void dc_flush_range(const void *addr, u32 len);
void dc_invalidate_range(const void *addr, u32 len);

#endif /* DSI_COMMON_CACHE_H */
