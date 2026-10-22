/*
 * src/common/cache.c
 *
 * CP15 cache maintenance. See include/common/cache.h.
 *
 * The ARM946E-S cache is maintained by "set/way" operations. A full clean is
 * performed by iterating over every way and index. This mirrors the reference
 * loop in FUN_0001cae0, written as a bounded, explicit loop.
 */
#include "common/cache.h"

/*
 * The ARM946E-S (ARMv5TE) has CP15 cache-maintenance operations; the ARM7TDMI
 * (ARMv4T) has neither a data cache nor CP15. Guard the MCR instructions so the
 * same source builds for both cores.
 */
#if defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__)
#define DSI_HAS_CP15_CACHE 1
#else
#define DSI_HAS_CP15_CACHE 0
#endif

#define CACHE_LINE_WORDS   8u    /* 32-byte lines                     */
#define CACHE_SETS         64u   /* index step count (0x400 / 0x20)   */

#if DSI_HAS_CP15_CACHE

static inline void cp15_clean_dcache_index(u32 value)
{
    __asm__ volatile ("mcr p15, 0, %0, c7, c10, 2" :: "r"(value) : "memory");
}

static inline void cp15_clean_dcache_all(void)
{
    __asm__ volatile ("mcr p15, 0, %0, c7, c10, 0" :: "r"(0u) : "memory");
}

static inline void cp15_invalidate_dcache_all(void)
{
    __asm__ volatile ("mcr p15, 0, %0, c7, c6, 0" :: "r"(0u) : "memory");
}

static inline void cp15_invalidate_icache_all(void)
{
    __asm__ volatile ("mcr p15, 0, %0, c7, c5, 0" :: "r"(0u) : "memory");
}

static inline void cp15_dsb(void)
{
    __asm__ volatile ("mcr p15, 0, %0, c7, c10, 4" :: "r"(0u) : "memory");
}

/*
 * @ref FUN_0001cae0 — clean by index across all ways (0..3) and sets (0..63).
 * Way-stride is 0x40000000 and index-stride 0x20 (one cache line).
 */
void cache_clean_all(void)
{
    u32 way;

    for (way = 0u; way < 4u; way++) {
        u32 base = way * 0x40000000u;
        u32 idx;

        for (idx = 0u; idx < CACHE_SETS; idx++) {
            cp15_clean_dcache_index(base + idx * CACHE_LINE_WORDS * 4u);
        }
    }
    cp15_dsb();
}

void cache_invalidate_all(void)
{
    cp15_dsb();
    cp15_invalidate_dcache_all();
    cp15_invalidate_icache_all();
    cp15_dsb();
}

void cache_flush_all(void)
{
    /* Clean first so dirty lines are written back, then invalidate. */
    cache_clean_all();
    cache_invalidate_all();
}

void dc_flush_range(const void *addr, u32 len)
{
    (void)addr;
    (void)len;
    cache_flush_all();
}

void dc_invalidate_range(const void *addr, u32 len)
{
    (void)addr;
    (void)len;
    cache_invalidate_all();
}

#else /* !DSI_HAS_CP15_CACHE — ARM7TDMI has no cache to maintain. */

void cache_clean_all(void)      { }
void cache_invalidate_all(void) { }
void cache_flush_all(void)      { }
void dc_flush_range(const void *addr, u32 len)      { (void)addr; (void)len; }
void dc_invalidate_range(const void *addr, u32 len) { (void)addr; (void)len; }

#endif /* DSI_HAS_CP15_CACHE */
