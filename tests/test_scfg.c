/*
 * tests/test_scfg.c
 *
 * Host-side unit tests for the hardware-independent parts of the SCFG/MBK
 * support code in src/arm9/scfg.c: the SCFG_A9ROM mode decode, the SCFG_CLK
 * clock decode, the SCFG_EXT RAM-window decode and the MBK slot packer.
 *
 * The register-touching functions (scfg_is_twl_mode, scfg_read_state, ...) are
 * not exercised here because they dereference absolute MMIO addresses; only
 * the pure decoders are.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>

#include "arm9/scfg.h"

static void test_mode_decode(void)
{
    /* GBATEK canonical test: (a9rom & 3) == 1 => DSi mode. */
    assert(scfg_mode_from_a9rom(0x0001u) == SCFG_CONSOLE_DSI);
    assert(scfg_mode_from_a9rom(0x0101u) == SCFG_CONSOLE_DSI);
    assert(scfg_mode_from_a9rom(0x0003u) == SCFG_CONSOLE_NDS);
    assert(scfg_mode_from_a9rom(0x0303u) == SCFG_CONSOLE_NDS);
    assert(scfg_mode_from_a9rom(0x0000u) == SCFG_CONSOLE_NDS);
    assert(scfg_mode_from_a9rom(0x0002u) == SCFG_CONSOLE_NDS);
    assert(scfg_mode_from_a9rom(0xFFFFu) == SCFG_CONSOLE_NDS);
}

static void test_clock_decode(void)
{
    /* bit0 = 1 selects the 134.06 MHz TWL clock. */
    assert(scfg_arm9_clock_hz(0x0001u) == SCFG_ARM9_CLK_TWL_HZ);
    assert(scfg_arm9_clock_hz(0x0187u) == SCFG_ARM9_CLK_TWL_HZ);
    /* bit0 = 0 is the 67.03 MHz NITRO clock. */
    assert(scfg_arm9_clock_hz(0x0000u) == SCFG_ARM9_CLK_NITRO_HZ);
    assert(scfg_arm9_clock_hz(0x0086u) == SCFG_ARM9_CLK_NITRO_HZ);
    assert(scfg_arm9_clock_hz(0xFEFEu) == SCFG_ARM9_CLK_NITRO_HZ);

    assert(SCFG_ARM9_CLK_NITRO_MHZ == 67u);
    assert(SCFG_ARM9_CLK_TWL_MHZ == 134u);
}

static void test_ext_ram_decode(void)
{
    /* SCFG_EXT bits 14-15: 0/1 => 4 MiB, 2 => 16 MiB, 3 => 32 MiB. */
    assert(scfg_ext_ram_bytes(0x00000000u) == 0x00400000u);
    assert(scfg_ext_ram_bytes(SCFG_EXT_RAM_4MB) == 0x00400000u);
    assert(scfg_ext_ram_bytes((1u << 14)) == 0x00400000u);
    assert(scfg_ext_ram_bytes(SCFG_EXT_RAM_16MB) == 0x01000000u);
    assert(scfg_ext_ram_bytes(SCFG_EXT_RAM_32MB) == 0x02000000u);

    /* The reference boot value selects the 32 MiB window (debug map). */
    assert(scfg_ext_ram_bytes(SCFG_EXT_BOOT_VALUE) == 0x02000000u);
}

static void test_mbk_slot_packer(void)
{
    /* WRAM-A: master is a single bit; offsets 0..3. */
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 0u, true) == 0x80u);
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 3u, true) == 0x8Cu);
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 0u, true) == 0x81u);
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 3u, true) == 0x8Du);
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 0u, false) == 0x00u);

    /* WRAM-B/C accept the DSP master and offsets 0..7. */
    assert(mbk_slot_entry(MBK_SLOT_MASTER_DSP, 7u, true) == 0x9Eu);
    assert(mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 7u, true) == 0x9Cu);
}

static void test_mbk_defaults(void)
{
    /* MBK1: WRAM-A slots 0..3 all master ARM7, offsets 0..3, enabled. */
    assert((u8)(MBK1_DEFAULT_VALUE >> 0)  == mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 0u, true));
    assert((u8)(MBK1_DEFAULT_VALUE >> 8)  == mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 1u, true));
    assert((u8)(MBK1_DEFAULT_VALUE >> 16) == mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 2u, true));
    assert((u8)(MBK1_DEFAULT_VALUE >> 24) == mbk_slot_entry(MBK_SLOT_MASTER_ARM7, 3u, true));

    /* MBK2/MBK3: WRAM-B slots 0..7 all master ARM9, offsets 0..7. */
    assert((u8)(MBK2_DEFAULT_VALUE >> 0)  == mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 0u, true));
    assert((u8)(MBK3_DEFAULT_VALUE >> 24) == mbk_slot_entry(MBK_SLOT_MASTER_ARM9, 7u, true));

    /* MBK9 protection fields are disjoint and cover MBK1..MBK5. */
    assert((MBK9_WRAM_A_PROTECT & MBK9_WRAM_B_PROTECT) == 0u);
    assert((MBK9_WRAM_B_PROTECT & MBK9_WRAM_C_PROTECT) == 0u);
    assert((MBK9_WRAM_A_PROTECT & MBK9_WRAM_C_PROTECT) == 0u);
    assert(MBK9_PROTECT_MASK ==
           (MBK9_WRAM_A_PROTECT | MBK9_WRAM_B_PROTECT | MBK9_WRAM_C_PROTECT));
}

int main(void)
{
    test_mode_decode();
    test_clock_decode();
    test_ext_ram_decode();
    test_mbk_slot_packer();
    test_mbk_defaults();

    puts("test_scfg: all assertions passed");
    return 0;
}
