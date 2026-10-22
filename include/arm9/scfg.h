/*
 * include/arm9/scfg.h
 *
 * TWL SCFG (system configuration) and MBK (new shared WRAM) accessors.
 *
 * Reconstructed from the decompiled ARM9 image (see PLAN.md §0 and
 * docs/hardware_notes.md §9/§11/§14). Register addresses, the extended-RAM bit
 * and the exact boot values (SCFG_EXT = 0x8307F100, SCFG_CLK &= 0xFEFE) come
 * from the crt0 stub (`@ref 0x02200800`) and the stage-2 entry
 * (`@ref 0x0602572C`); the bit meanings are the public DSi register map
 * (GBATEK "DSi Control Registers (SCFG)" and "DSi New Shared WRAM").
 *
 * SAFETY: SCFG writes affect CPU clocks, SD/MMC ownership and whether the
 * upper 12 MiB of Main RAM is mapped; MBK writes re-map the NWRAM banks.
 * Every write is gated on scfg_is_twl_mode(), so on a legacy DS (where the
 * whole 0x04004000 page is unmapped) the functions degrade to read-only
 * no-ops rather than spamming the bus with writes to nonexistent registers.
 */
#ifndef DSI_ARM9_SCFG_H
#define DSI_ARM9_SCFG_H

#include "../common/types.h"
#include "../common/dsi_hw.h"

/* Console mode decoded from SCFG_A9ROM (GBATEK: (a9rom & 3) == 1 => DSi). */
typedef enum {
    SCFG_CONSOLE_NDS = 0,   /* legacy DS, or a DSi running an NDS title   */
    SCFG_CONSOLE_DSI = 1    /* DSi console running in DSi/TWL mode        */
} scfg_console_mode_t;

/* Snapshot of the SCFG/MBK state shown by the register-inspection view. */
typedef struct {
    bool    accessible;     /* the 0x04004000 page is backed by hardware  */
    bool    twl_mode;       /* DSi running in DSi mode                    */
    u16     a9rom;          /* raw SCFG_A9ROM (16-bit)                    */
    u16     clk;            /* raw SCFG_CLK (16-bit)                      */
    u32     ext;            /* raw SCFG_EXT (32-bit)                      */
    u32     mbk[MBK_COUNT]; /* raw MBK1..MBK9                             */
    bool    extended_ram;   /* Main RAM window is 16/32 MiB               */
    bool    ndma;           /* NDMA engine accessible                     */
    bool    scfg_unlocked;  /* SCFG_EXT.bit31 (empty regs accessible)     */
    u32     arm9_hz;        /* decoded ARM9 core clock in Hz              */
} scfg_state_t;

/* ---- Pure decoders (host-testable) ------------------------------------ */

/* Decode the console mode from a raw SCFG_A9ROM value. */
scfg_console_mode_t scfg_mode_from_a9rom(u16 a9rom);

/* Decode the ARM9 core clock (Hz) from a raw SCFG_CLK value. */
u32 scfg_arm9_clock_hz(u16 clk);

/* Decode the Main RAM limit (bytes) from a raw SCFG_EXT value. */
u32 scfg_ext_ram_bytes(u32 ext);

/* ---- Read-only snapshots ---------------------------------------------- */

u32 scfg_read_rom(void);   /* SCFG_A9ROM, zero-extended                  */
u32 scfg_read_clk(void);   /* SCFG_CLK, zero-extended                    */
u32 scfg_read_ext(void);   /* SCFG_EXT                                   */

/*
 * Runtime TWL-mode probe.
 *
 * Reads SCFG_A9ROM (0x04004000, read-only on ARM9) and applies the canonical
 * GBATEK test: bits 0-1 equal 0x01 means "DSi console running in DSi mode".
 *
 * An explicit 0x03 means the DSi is running an NDS title (DS compatibility
 * mode) and is authoritative: the probe returns false even though
 * SCFG_EXT.bit31 (the SCFG/MBK access gate) is typically still set. Only when
 * A9ROM reads 0x00 — the page is hidden on a legacy DS, or bootstage-1 has not
 * latched the mapping yet — does the probe fall back to SCFG_EXT.bit31.
 */
bool scfg_is_twl_mode(void);

/* True when the 0x04004000 SCFG/MBK page can actually be read/written. */
bool scfg_hardware_accessible(void);

/* Force the console mode probe to a known result (diagnostics/tests). */
void scfg_set_mode_override(scfg_console_mode_t mode);

/* Fill `out` with the current SCFG/MBK snapshot (safe on a legacy DS). */
void scfg_read_state(scfg_state_t *out);

/* True when SCFG_EXT reports the extended RAM window as enabled. */
bool scfg_extended_ram_is_enabled(void);

/* Program one MBK (New Shared WRAM) owner register. */
bool scfg_set_mbk(u8 index, u32 value);

/* ---- Configuration ---------------------------------------------------- */

/*
 * Enable the extended (16 MiB) Main RAM window and grant the ARM9 NDMA/SD
 * access rights. Sets SCFG_EXT to the 16 MiB RAM mode
 * (SCFG_EXT_RAM_16MB) with SCFG_EXT_NDMA_ARM9 | SCFG_EXT_DSP_ARM9.
 * No-op on a legacy DS (see scfg_is_twl_mode()).
 *
 * Callers MUST be running from a region that is mapped regardless of the
 * extended window (ITCM or the low 4 MiB) before calling this.
 */
void scfg_enable_extended_ram(void);

/*
 * Apply the exact configuration observed in the reference ARM9 boot path:
 * SCFG_EXT = 0x8307F100 (extended RAM on) and SCFG_CLK &= 0xFEFE.
 *
 * @ref crt0  (0x02200800)
 * @ref entry (0x0602572C)
 */
void scfg_apply_boot_config(void);

/*
 * Ensure SCFG_EXT.bit31 is set so the SCFG/MBK register window is accessible.
 * Returns true when it is (already) set, false on a legacy DS (no-op).
 */
bool scfg_unlock(void);

/*
 * Configure the NWRAM slot-ownership registers MBK1..MBK5 with the canonical
 * DSi mapping (WRAM-A -> ARM7, WRAM-B/C -> ARM9). MBK9 is read-only on the
 * ARM9 and is honoured: protected banks are left untouched. Returns the number
 * of slot registers that actually stuck (0 on a legacy DS or fully-protected
 * configuration).
 */
u8 mbk_init(void);

/*
 * One-shot DSi bring-up: verify TWL mode, unlock the SCFG/MBK window, apply
 * the reference SCFG_EXT/SCFG_CLK configuration and initialise the NWRAM
 * banks. Returns true if the DSi hardware was configured; false (a complete
 * no-op) on a legacy DS.
 */
bool scfg_init(void);

#endif /* DSI_ARM9_SCFG_H */
