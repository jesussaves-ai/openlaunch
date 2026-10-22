/*
 * include/arm9/scfg.h
 *
 * TWL SCFG (system configuration) accessors.
 *
 * Reconstructed from the decompiled ARM9 image (see PLAN.md §0 and
 * docs/hardware_notes.md §9/§11). Register addresses, the extended-RAM bit and
 * the exact boot values (SCFG_EXT = 0x8307F100, SCFG_CLK &= 0xFEFE) come from
 * the crt0 stub (`@ref 0x02200800`) and the stage-2 entry (`@ref 0x0602572C`).
 *
 * SAFETY: SCFG writes affect CPU clocks, SD/MMC ownership and whether the
 * upper 12 MiB of Main RAM is mapped. They are exposed as explicit, reversible
 * functions rather than performed implicitly during early boot.
 */
#ifndef DSI_ARM9_SCFG_H
#define DSI_ARM9_SCFG_H

#include "../common/types.h"

/* Read-only snapshots. */
u32 scfg_read_rom(void);
u32 scfg_read_clk(void);
u32 scfg_read_ext(void);

/*
 * Runtime TWL-mode probe.
 *
 * Reads SCFG_A9ROM (0x04004000), the DSi-only ARM9 ROM-status register, and
 * returns true when the console reports the DSi-mode signature (0x01) or the
 * extended-feature register is otherwise backed by hardware. On a legacy DS
 * (where the whole 0x04004000 page is unmapped) it returns false, letting the
 * caller skip DSi-only register writes.
 */
bool scfg_is_twl_mode(void);

/*
 * Enable the extended (16 MiB) Main RAM window and grant the ARM9 NDMA/SD
 * access rights. Sets SCFG_EXT to the 16 MiB RAM mode
 * (SCFG_EXT_RAM_16MB) with SCFG_EXT_NDMA_ARM9 | SCFG_EXT_SDIO_ENABLE.
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

/* True when SCFG_EXT reports the extended RAM window as enabled. */
bool scfg_extended_ram_is_enabled(void);

/*
 * Program one MBK (New Shared WRAM) owner register.
 *
 * @param index 1..9  (MBK6..MBK8 are master-permission locked)
 * @param value raw slave/owner configuration
 * @return true if the write was accepted (MBK6..8 may silently retain).
 */
bool scfg_set_mbk(u8 index, u32 value);

#endif /* DSI_ARM9_SCFG_H */
