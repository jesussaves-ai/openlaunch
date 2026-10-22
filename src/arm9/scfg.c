/*
 * src/arm9/scfg.c
 *
 * TWL SCFG accessors. See include/arm9/scfg.h and AGENTS.md §3.
 *
 * The reference ARM9 crt0 programs SCFG_EXT (0x04004008) with the exact value
 * 0x8307F100 and later masks SCFG_CLK (0x04004004) with 0xFEFE
 * (@ref crt0 0x02200800, @ref entry 0x0602572C). Those constants are exposed
 * through scfg_apply_boot_config().
 *
 * The whole 0x04004000 page is DSi-only. On a legacy DS it is unmapped, so
 * every write below is gated by scfg_is_twl_mode() to avoid spamming the bus
 * with accesses to nonexistent registers when the ROM is launched in DS mode.
 */
#include "arm9/scfg.h"
#include "common/dsi_hw.h"

static inline void scfg_memory_barrier(void)
{
#if defined(__arm__) || defined(__thumb__)
    __asm__ volatile ("mcr p15, 0, %0, c7, c10, 4" :: "r"(0u) : "memory");
#endif
}

/*
 * Runtime "am I on a DSi in DSi mode?" probe.
 *
 * SCFG_A9ROM (0x04004000) is a read-only ROM-status register on the ARM9.
 * GBATEK documents 0x01 as the canonical "DSi console running in DSi mode"
 * signature. On a legacy DS (or a DSi running an NDS title) the page is
 * unmapped/disabled and reads back 0, so the fallback also checks whether the
 * extended-feature register is backed by hardware (bit 31 set).
 */
bool scfg_is_twl_mode(void)
{
    if ((REG32(SCFG_A9ROM) & 0xFFu) == SCFG_A9ROM_DSI_MODE) {
        return true;
    }
    return (REG32(SCFG_EXT) & SCFG_EXT_SCFG_ACCESS) != 0u;
}

u32 scfg_read_rom(void)
{
    return REG32(SCFG_A9ROM);
}

u32 scfg_read_clk(void)
{
    return REG32(SCFG_CLK);
}

u32 scfg_read_ext(void)
{
    return REG32(SCFG_EXT);
}

bool scfg_extended_ram_is_enabled(void)
{
    /* Bits 14-15 select the Main RAM window; >= 2 means the 16 MiB map. */
    return (REG32(SCFG_EXT) & SCFG_EXT_RAM_MASK) >= SCFG_EXT_RAM_16MB;
}

/*
 * Apply the reference boot configuration verbatim:
 *
 *   SCFG_EXT = 0x8307F100    (32 MiB RAM window, NDMA + SCFG access)
 *   SCFG_CLK = SCFG_CLK & 0xFEFE
 *
 * AGENTS.md §5: only the documented SCFG bits are touched. The writes only
 * happen on a DSi; on a legacy DS the function is a no-op.
 */
void scfg_apply_boot_config(void)
{
    if (!scfg_is_twl_mode()) {
        return;
    }

    REG32(SCFG_EXT) = SCFG_EXT_BOOT_VALUE;
    REG32(SCFG_CLK) = REG32(SCFG_CLK) & 0xFEFEu;

    scfg_memory_barrier();
}

void scfg_enable_extended_ram(void)
{
    u32 ext;

    if (!scfg_is_twl_mode()) {
        return;
    }

    ext = REG32(SCFG_EXT);
    ext &= ~SCFG_EXT_RAM_MASK;
    ext |= (SCFG_EXT_RAM_16MB | SCFG_EXT_NDMA_ARM9 | SCFG_EXT_SDIO_ENABLE);
    REG32(SCFG_EXT) = ext;

    scfg_memory_barrier();
}

bool scfg_set_mbk(u8 index, u32 value)
{
    u32 addr;

    if (index < 1u || index > 9u) {
        return false;
    }
    if (!scfg_is_twl_mode()) {
        return false;
    }

    addr = MBK_BASE + (u32)(index - 1u) * 4u;
    REG32(addr) = value;

    /* MBK6..MBK8 are master-permission locked; report whether it stuck. */
    return REG32(addr) == value;
}
