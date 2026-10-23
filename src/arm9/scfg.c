/*
 * src/arm9/scfg.c
 *
 * TWL SCFG accessors. See include/arm9/scfg.h and the project conventions
 *
 * The reference ARM9 crt0 programs SCFG_EXT (0x04004008) with the exact value
 * 0x8307F100 and later masks SCFG_CLK (0x04004004) with 0xFEFE
 * (@ref crt0 0x02200800, @ref entry 0x0602572C). Those constants are exposed
 * through scfg_apply_boot_config(); the NWRAM banks (MBK1..MBK9) are
 * configured by mbk_init()/scfg_init().
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

/* Optional forced mode for diagnostics. `s_override_set` selects it. */
static bool               s_override_set = false;
static scfg_console_mode_t s_override     = SCFG_CONSOLE_NDS;

/* ------------------------------------------------------------------ */
/* Pure decoders (host-testable).                                      */
/* ------------------------------------------------------------------ */

scfg_console_mode_t scfg_mode_from_a9rom(u16 a9rom)
{
    /* GBATEK: "if ([4004000h] AND 03h)=01h then DSi_mode else NDS_mode". */
    return ((a9rom & SCFG_A9ROM_MODE_MASK) == SCFG_A9ROM_DSI_MODE)
               ? SCFG_CONSOLE_DSI
               : SCFG_CONSOLE_NDS;
}

u32 scfg_arm9_clock_hz(u16 clk)
{
    return ((clk & SCFG_CLK_ARM9_134MHZ) != 0u)
               ? SCFG_ARM9_CLK_TWL_HZ
               : SCFG_ARM9_CLK_NITRO_HZ;
}

u32 scfg_ext_ram_bytes(u32 ext)
{
    switch ((ext & SCFG_EXT_RAM_MASK) >> 14) {
    case 0u:                 /* 0 = 4 MiB                           */
    case 1u:                 /* 1 = 4 MiB                           */
        return 0x00400000u;
    case 2u:                 /* 2 = 16 MiB                          */
        return 0x01000000u;
    default:                 /* 3 = 32 MiB (debug consoles)         */
        return 0x02000000u;
    }
}

/* ------------------------------------------------------------------ */
/* Runtime detection.                                                  */
/* ------------------------------------------------------------------ */

void scfg_set_mode_override(scfg_console_mode_t mode)
{
    s_override = mode;
    s_override_set = true;
}

bool scfg_is_twl_mode(void)
{
    u16 a9rom;

    if (s_override_set) {
        return s_override == SCFG_CONSOLE_DSI;
    }

    a9rom = (u16)(REG16(SCFG_A9ROM) & SCFG_A9ROM_MODE_MASK);
    if (a9rom == SCFG_A9ROM_DSI_MODE) {
        return true;
    }
    if (a9rom == SCFG_A9ROM_NDS_MODE) {
        /*
         * The DSi is running an NDS title in DS compatibility mode. A9ROM is
         * authoritative even though SCFG_EXT.bit31 (the access gate) is still
         * typically set by the firmware, so do NOT fall through here.
         */
        return false;
    }

    /*
     * A9ROM == 0x00: the page is hidden (legacy DS) or bootstage-1 has not
     * latched the mapping yet. Fall back to the SCFG/MBK access gate, which
     * the DSi firmware enables in DSi mode.
     */
    return (REG32(SCFG_EXT) & SCFG_EXT_SCFG_ACCESS) != 0u;
}

bool scfg_hardware_accessible(void)
{
    if (s_override_set) {
        return s_override == SCFG_CONSOLE_DSI;
    }
    if ((REG16(SCFG_A9ROM) & SCFG_A9ROM_MODE_MASK) != 0u) {
        return true;   /* either 0x01 (DSi) or 0x03 (DSi in NDS mode) */
    }
    return (REG32(SCFG_EXT) & SCFG_EXT_SCFG_ACCESS) != 0u;
}

u32 scfg_read_rom(void)
{
    /* SCFG_A9ROM is a 16-bit register; read it at its documented width. */
    return (u32)REG16(SCFG_A9ROM);
}

u32 scfg_read_clk(void)
{
    return (u32)REG16(SCFG_CLK);
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

void scfg_read_state(scfg_state_t *out)
{
    u32 i;

    if (out == NULL) {
        return;
    }

    out->accessible     = false;
    out->twl_mode       = false;
    out->a9rom          = 0u;
    out->clk            = 0u;
    out->ext            = 0u;
    out->extended_ram   = false;
    out->ndma           = false;
    out->scfg_unlocked  = false;
    out->arm9_hz        = SCFG_ARM9_CLK_NITRO_HZ;
    for (i = 0u; i < MBK_COUNT; i++) {
        out->mbk[i] = 0u;
    }

    if (!scfg_hardware_accessible()) {
        return;   /* legacy DS: leave the snapshot empty */
    }

    out->accessible    = true;
    out->a9rom         = (u16)REG16(SCFG_A9ROM);
    out->clk           = REG16(SCFG_CLK);
    out->ext           = REG32(SCFG_EXT);
    out->twl_mode      = scfg_is_twl_mode();
    out->extended_ram  = ((out->ext & SCFG_EXT_RAM_MASK) >= SCFG_EXT_RAM_16MB);
    out->ndma          = (out->ext & SCFG_EXT_NDMA_ARM9) != 0u;
    out->scfg_unlocked = (out->ext & SCFG_EXT_SCFG_ACCESS) != 0u;
    out->arm9_hz       = scfg_arm9_clock_hz(out->clk);

    for (i = 0u; i < MBK_COUNT; i++) {
        out->mbk[i] = REG32(MBK_BASE + i * 4u);
    }
}

/* ------------------------------------------------------------------ */
/* Configuration.                                                      */
/* ------------------------------------------------------------------ */

/*
 * Apply the reference boot configuration verbatim:
 *
 *   SCFG_EXT = 0x8307F100    (32 MiB RAM window, NDMA + SCFG access)
 *   SCFG_CLK = SCFG_CLK & 0xFEFE
 *
 * the project conventions: only the documented SCFG bits are touched. The writes only
 * happen on a DSi; on a legacy DS the function is a no-op.
 */
void scfg_apply_boot_config(void)
{
    if (!scfg_is_twl_mode()) {
        return;
    }

    REG32(SCFG_EXT) = SCFG_EXT_BOOT_VALUE;
    REG16(SCFG_CLK) = (u16)(REG16(SCFG_CLK) & 0xFEFEu);

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
    ext |= (SCFG_EXT_RAM_16MB | SCFG_EXT_NDMA_ARM9 | SCFG_EXT_DSP_ARM9);
    REG32(SCFG_EXT) = ext;

    scfg_memory_barrier();
}

bool scfg_set_mbk(u8 index, u32 value)
{
    u32 addr;

    if (index < 1u || index > MBK_COUNT) {
        return false;
    }
    if (!scfg_is_twl_mode()) {
        return false;
    }

    addr = MBK_BASE + (u32)(index - 1u) * 4u;
    REG32(addr) = value;

    /* MBK1..MBK5 may be write-protected via MBK9; report whether it stuck. */
    return REG32(addr) == value;
}

bool scfg_unlock(void)
{
    u32 ext;

    if (!scfg_is_twl_mode()) {
        return false;
    }

    ext = REG32(SCFG_EXT);
    if ((ext & SCFG_EXT_SCFG_ACCESS) == 0u) {
        REG32(SCFG_EXT) = ext | SCFG_EXT_SCFG_ACCESS;
        scfg_memory_barrier();
    }
    return true;
}

/*
 * Configure the NWRAM slot-ownership registers. MBK9 is read-only on ARM9 and
 * reports which of MBK1..MBK5 the ARM7 has pinned; protected groups are left
 * alone rather than forcing a losing write.
 */
u8 mbk_init(void)
{
    static const u32 defaults[5] = {
        MBK1_DEFAULT_VALUE,   /* WRAM-A slots 0..3 -> ARM7 */
        MBK2_DEFAULT_VALUE,   /* WRAM-B slots 0..3 -> ARM9 */
        MBK3_DEFAULT_VALUE,   /* WRAM-B slots 4..7 -> ARM9 */
        MBK4_DEFAULT_VALUE,   /* WRAM-C slots 0..3 -> ARM9 */
        MBK5_DEFAULT_VALUE,   /* WRAM-C slots 4..7 -> ARM9 */
    };
    u32 protect;
    u8 applied = 0u;

    if (!scfg_is_twl_mode()) {
        return 0u;
    }

    protect = REG32(MBK9);

    if ((protect & MBK9_WRAM_A_PROTECT) == 0u) {
        if (scfg_set_mbk(1u, defaults[0])) { applied++; }
    }
    if ((protect & MBK9_WRAM_B_PROTECT) == 0u) {
        if (scfg_set_mbk(2u, defaults[1])) { applied++; }
        if (scfg_set_mbk(3u, defaults[2])) { applied++; }
    }
    if ((protect & MBK9_WRAM_C_PROTECT) == 0u) {
        if (scfg_set_mbk(4u, defaults[3])) { applied++; }
        if (scfg_set_mbk(5u, defaults[4])) { applied++; }
    }

    return applied;
}

bool scfg_init(void)
{
    if (!scfg_is_twl_mode()) {
        return false;
    }

    (void)scfg_unlock();
    scfg_apply_boot_config();
    (void)mbk_init();

    scfg_memory_barrier();
    return true;
}
