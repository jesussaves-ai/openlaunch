/*
 * include/common/dsi_hw.h
 *
 * Nintendo DSi (TWL) memory map and core I/O register definitions.
 *
 * All addresses/bit meanings in this header are justified in
 * docs/hardware_notes.md (derived from the reference disassembly and from the
 * public DSi register map). Do not invent registers here.
 *
 * the project conventions: every field is volatile and reached through a typed pointer or
 * the REGx() macro family from types.h.
 */
#ifndef DSI_COMMON_DSI_HW_H
#define DSI_COMMON_DSI_HW_H

#include "types.h"

/* ===================================================================== */
/* Memory map (TWL mode) — the project conventions                       */
/* ===================================================================== */
#define ITCM_BASE        0x01FF8000u   /* 32 KiB  zero-waitstate code       */
#define MAIN_RAM_BASE    0x02000000u   /* 16 MiB  (low 4 MiB in NDS mode)   */
#define SHARED_WRAM_BASE 0x03000000u   /* MBK-mapped NWRAM                  */
#define ARM7_WRAM_BASE   0x03800000u   /* 64 KiB  ARM7 code/stack           */
#define ARM7_WRAM_SIZE   0x00010000u
#define DTCM_BASE        0x0B000000u   /* 16 KiB  ARM9 data/stack (no DMA)  */
#define DTCM_SIZE        0x00004000u

/* Boot load addresses taken from the reference NDS header. */
#define ARM9_LOAD_ADDR   0x02200000u
#define ARM9_ENTRY_ADDR  0x02200800u
#define ARM7_LOAD_ADDR   0x02380000u
#define ARM7_ENTRY_ADDR  0x02380000u

/* ===================================================================== */
/* TWL system configuration (SCFG) — the project conventions             */
/* ===================================================================== */
/*
 * The whole 0x04004000 page only exists on a DSi. On a legacy DS it is
 * unmapped and reads back zero, which is what the runtime TWL probe in
 * src/arm9/scfg.c::scfg_is_twl_mode() relies on.
 *
 * Access widths (GBATEK "DSi I/O Map" / "DSi Control Registers"):
 *   0x04004000  16-bit  ARM9: SCFG_A9ROM (read-only ROM status)
 *                       ARM7: SCFG_ROM   (R/W, set-once ROM control)
 *   0x04004004  16-bit  SCFG_CLK (ARM9) / SCFG_CLK7 (ARM7)
 *   0x04004006  16-bit  SCFG_RST (ARM9) / SCFG_JTAG (ARM7)
 *   0x04004008  32-bit  SCFG_EXT9 (ARM9) / SCFG_EXT7 (ARM7)
 *
 * NOTE: the previous revision placed SCFG_A9ROM at 0x04004200, which is
 * actually the DSi camera interface. The ARM9 ROM-status register lives at
 * 0x04004000 (it aliases SCFG_ROM, which is R/W on the ARM7 side).
 */
#define SCFG_BASE        0x04004000u
#define SCFG_A9ROM       (SCFG_BASE + 0x00u)  /* ARM9: ROM status (R)      */
#define SCFG_ROM         (SCFG_BASE + 0x00u)  /* ARM7: ROM control (R/W)   */
#define SCFG_CLK         (SCFG_BASE + 0x04u)  /* CPU/SD clock dividers     */
#define SCFG_RST         (SCFG_BASE + 0x06u)  /* new-block reset           */
#define SCFG_EXT         (SCFG_BASE + 0x08u)  /* extended features         */

/* SCFG_A9ROM (read-only) values (GBATEK DSi Control Registers). */
#define SCFG_A9ROM_MODE_MASK    0x03u   /* bits 0-1 select the ROM mapping  */
#define SCFG_A9ROM_DSI_MODE     0x01u   /* DSi console running in DSi mode  */
#define SCFG_A9ROM_NDS_MODE     0x03u   /* NDS BIOS mapped (legacy DS mode) */

/*
 * SCFG_CLK (0x04004004) bit fields (GBATEK "DSi Control Registers (SCFG)").
 * The ARM9 read/write view; SCFG_CLK7 on the ARM7 has a different layout.
 */
#define SCFG_CLK_ARM9_134MHZ    BIT(0)  /* ARM9 CPU: 1=134.06, 0=67.03 MHz  */
#define SCFG_CLK_DSP            BIT(1)  /* Teak DSP block clock             */
#define SCFG_CLK_CAMERA         BIT(2)  /* camera interface clock           */
#define SCFG_CLK_NWRAM          BIT(7)  /* new shared RAM clock (via ARM7)  */
#define SCFG_CLK_CAMERA_EXT     BIT(8)  /* camera external clock (16.76 MHz)*/

/*
 * SCFG_CLK7 (0x04004004) as seen from the ARM7 — a different layout from the
 * ARM9 SCFG_CLK above (GBATEK "DSi Control Registers (SCFG)"):
 *
 *   bit 0  SD/MMC block clock        (0=stop, 1=run)
 *   bit 1  unknown, backlight-related
 *   bit 2  unknown
 *   bit 7  new shared RAM clock
 *   bit 8  touchscreen clock         (needed for touchscreen input)
 *
 * The SD/MMC clock must run before the TMIO block at 0x04004800 is touched.
 */
#define SCFG_CLK7_SDMMC         BIT(0)  /* SD/MMC block clock (ARM7 view)    */
#define SCFG_CLK7_UNKNOWN1      BIT(1)  /* backlight-related                 */
#define SCFG_CLK7_UNKNOWN2      BIT(2)
#define SCFG_CLK7_NWRAM         BIT(7)  /* new shared RAM clock (ARM7 view)  */
#define SCFG_CLK7_TOUCH         BIT(8)  /* touchscreen clock (ARM7 view)     */

/* Nominal ARM9 core clocks selected by SCFG_CLK.bit0. */
#define SCFG_ARM9_CLK_NITRO_HZ  67028000u   /* 67.03 MHz (NITRO/DS)         */
#define SCFG_ARM9_CLK_TWL_HZ    134056000u  /* 134.06 MHz (TWL/DSi)         */
#define SCFG_ARM9_CLK_NITRO_MHZ 67u
#define SCFG_ARM9_CLK_TWL_MHZ   134u

/*
 * SCFG_EXT9 (ARM9) / SCFG_EXT7 (ARM7) bit fields. The positions below are
 * identical on both cores; only the "set via ARM9/ARM7" direction differs.
 */
#define SCFG_EXT_RAM_MASK       (3u << 14) /* Main Memory RAM limit field    */
#define SCFG_EXT_RAM_4MB        (0u << 14) /* 4 MiB  (legacy DS)             */
#define SCFG_EXT_RAM_16MB       (2u << 14) /* 16 MiB (DSi)                   */
#define SCFG_EXT_RAM_32MB       (3u << 14) /* 32 MiB (DSi debug only)        */
#define SCFG_EXT_NDMA_ARM9      BIT(16)    /* access to NDMA (0x04004100)    */
#define SCFG_EXT_CAMERA_ARM9    BIT(17)    /* ARM9: camera interface         */
#define SCFG_EXT_DSP_ARM9       BIT(18)    /* ARM9: Teak DSP block           */

/*
 * SCFG_EXT7 (0x04004008) ARM7-specific access gates (GBATEK "DSi Control
 * Registers (SCFG)"). Bits 7/12-15 are read-only mirrors of the ARM9 settings;
 * bits 18/19 are the SD/MMC and SDIO/Wi-Fi register-window enables and are the
 * ones the ARM7 must set before touching 0x04004800 / 0x04004A00.
 */
#define SCFG_EXT7_SDMMC_ACCESS  BIT(18)    /* access to 0x04004800 (SD/MMC)  */
#define SCFG_EXT7_SDIO_WIFI     BIT(19)    /* access to 0x04004A00 (SDIO)    */
#define SCFG_EXT_NWRAM_ACCESS   BIT(25)    /* R: new shared RAM enabled      */
#define SCFG_EXT_SCFG_ACCESS    BIT(31)    /* access to SCFG/MBK registers   */

/* Historical aliases (bit 18 is the SD/MMC window, not SDIO). */
#define SCFG_EXT_SDIO_ENABLE    SCFG_EXT7_SDMMC_ACCESS
#define SCFG_EXT_SDIO_WIFI      SCFG_EXT7_SDIO_WIFI

/*
 * Exact SCFG_EXT value programmed by the reference ARM9 crt0
 * (@ref crt0 @ 0x02200800, literal at 0x022008C4). Bits 14-15 select the
 * 32 MiB RAM window, bit 16 grants NDMA access and bit 31 unlocks the
 * SCFG/MBK registers. Kept as a named constant so the boot path is
 * reproducible.
 */
#define SCFG_EXT_BOOT_VALUE     0x8307F100u

/* New Shared WRAM ownership registers (MBK1..MBK9). */
#define MBK_BASE         0x04004040u
#define MBK1             (MBK_BASE + 0x00u)
#define MBK2             (MBK_BASE + 0x04u)
#define MBK3             (MBK_BASE + 0x08u)
#define MBK4             (MBK_BASE + 0x0Cu)
#define MBK5             (MBK_BASE + 0x10u)
#define MBK6             (MBK_BASE + 0x14u)  /* lock-restricted          */
#define MBK7             (MBK_BASE + 0x18u)  /* lock-restricted          */
#define MBK8             (MBK_BASE + 0x1Cu)  /* lock-restricted          */
#define MBK9             (MBK_BASE + 0x20u)  /* ARM9: read-only write-protect */

#define MBK_COUNT        9u

/*
 * MBK1..MBK5 "slot" byte fields (GBATEK "DSi New Shared WRAM").
 *
 * MBK1 packs four WRAM-A slots (64 KiB each); MBK2+MBK3 pack the eight
 * WRAM-B slots (32 KiB each); MBK4+MBK5 pack the eight WRAM-C slots.
 *
 *   bit 0     master  (WRAM-A: 0=ARM9, 1=ARM7)
 *                     (WRAM-B/C: 0=ARM9, 1=ARM7, 2 or 3=DSP)
 *   bits 2-4  offset  (0..7, i.e. destination slot, in 32/64 KiB units)
 *   bit 7     enable  (0=disabled, 1=enabled)
 */
#define MBK_SLOT_MASTER_ARM9  0x0u
#define MBK_SLOT_MASTER_ARM7  0x1u
#define MBK_SLOT_MASTER_DSP   0x2u
#define MBK_SLOT_MASTER_MASK  0x3u
#define MBK_SLOT_OFFSET_SHIFT 2u
#define MBK_SLOT_OFFSET_MASK  0x7u
#define MBK_SLOT_ENABLE       BIT(7)

/* Pack one MBK1..MBK5 slot byte. Pure; host-testable. */
static inline u8 mbk_slot_entry(u8 master, u8 offset, bool enable)
{
    return (u8)(((u32)master & MBK_SLOT_MASTER_MASK) |
                (((u32)offset & MBK_SLOT_OFFSET_MASK) << MBK_SLOT_OFFSET_SHIFT) |
                (enable ? MBK_SLOT_ENABLE : 0u));
}

/*
 * MBK9 (0x0404060) slot write-protect bits. Read-only on ARM9; when a field
 * is set the corresponding MBK1..MBK5 byte cannot be changed by the ARM9.
 */
#define MBK9_WRAM_A_PROTECT   (0x0000000Fu)  /* MBK1                        */
#define MBK9_WRAM_B_PROTECT   (0x0000FF00u)  /* MBK2/MBK3                   */
#define MBK9_WRAM_C_PROTECT   (0x00FF0000u)  /* MBK4/MBK5                   */
#define MBK9_PROTECT_MASK \
    (MBK9_WRAM_A_PROTECT | MBK9_WRAM_B_PROTECT | MBK9_WRAM_C_PROTECT)

/*
 * Canonical DSi NWRAM ownership: WRAM-A is handed to the ARM7 (it has only
 * 64 KiB of private WRAM) and mapped at 0x03000000; WRAM-B and WRAM-C stay
 * with the ARM9. These are the *slot-map* defaults; the firmware's address
 * windows (MBK6..MBK8) are left untouched by mbk_init().
 */
#define MBK1_DEFAULT_VALUE   0x8D898581u  /* WRAM-A slots 0..3 -> ARM7      */
#define MBK2_DEFAULT_VALUE   0x8C888480u  /* WRAM-B slots 0..3 -> ARM9      */
#define MBK3_DEFAULT_VALUE   0x9C989490u  /* WRAM-B slots 4..7 -> ARM9      */
#define MBK4_DEFAULT_VALUE   0x8C888480u  /* WRAM-C slots 0..3 -> ARM9      */
#define MBK5_DEFAULT_VALUE   0x9C989490u  /* WRAM-C slots 4..7 -> ARM9      */

/* ===================================================================== */
/* ARM9 interrupt controller — docs/hardware_notes.md §2                  */
/* ===================================================================== */
/*
 * Widths (GBATEK DS I/O map):
 *   IME is 16-bit on ARM9 and 32-bit on ARM7. Only bit 0 is defined, so a
 *   16-bit access is exact on both cores (the ARM7 upper half is unused).
 *   IE/IF/IE2/IF2 are 32-bit.
 */
#define IRQ_IME          0x04000208u   /* 16-bit (bit0 only)               */
#define IRQ_IE           0x04000210u   /* 32-bit                           */
#define IRQ_IF           0x04000214u   /* 32-bit                           */
#define IRQ_IE2          0x04000218u   /* 32-bit, second enable (ARM9)     */
#define IRQ_IF2          0x0400021Cu   /* 32-bit, second pending (ARM9)    */

/* EXMEMCNT — ARM9/ARM7 external memory & device access control (16-bit). */
#define EXMEMCNT         0x04000204u

/*
 * Power / boot-flag registers (GBATEK DS I/O map). These live in the
 * 0x04000300 page and MUST be accessed at their documented widths:
 *
 *   0x04000300  8-bit   POSTFLG  (bit0 = "booted" flag)
 *   0x04000301  8-bit   HALTCNT  (write 0x80 to halt the CPU)
 *   0x04000304  16-bit  POWCNT1  (ARM9: LCD / 2D / 3D enables)
 *                       POWCNT2  (ARM7: sound / wifi enables)
 *
 * A 32-bit store to 0x04000300 would clobber HALTCNT and can put the CPU to
 * sleep, which is exactly the failure the melonDS boot log reported.
 */
#define POSTFLG          0x04000300u   /* 8-bit  (ARM9 + ARM7)             */
#define HALTCNT          0x04000301u   /* 8-bit  (ARM7 only)               */
#define POWCNT1          0x04000304u   /* 16-bit (ARM9)                    */
#define POWCNT2          0x04000304u   /* 16-bit (ARM7)                    */

/* POWCNT1 bit fields (ARM9 graphics power). */
#define POWCNT1_LCD              BIT(0)
#define POWCNT1_ENGINE_A         BIT(1)
#define POWCNT1_ENGINE_B         BIT(2)
#define POWCNT1_ENGINE_3D        BIT(3)
#define POWCNT1_ENGINE_A_MASTER  BIT(8)
#define POWCNT1_ENGINE_B_MASTER  BIT(9)
#define POWCNT1_BOOT_VALUE       0x820Fu

/*
 * TWL secondary interrupt controller (FUN_00027028).
 *
 * FUN_00027028 reads [0x10] & [0x12] and writes the result back to [0x10],
 * i.e. "pending & enabled, then acknowledge the pending register".
 */
#define TWL_IRQ_BASE     0x04808000u
#define TWL_IRQ_IF       (TWL_IRQ_BASE + 0x10u)   /* pending / acknowledge   */
#define TWL_IRQ_IE       (TWL_IRQ_BASE + 0x12u)   /* enable mask             */

/* Standard ARM7 IRQ source bits. */
#define IRQ_VBLANK        BIT(0)
#define IRQ_HBLANK        BIT(1)
#define IRQ_VCOUNT        BIT(2)
#define IRQ_TIMER0        BIT(3)
#define IRQ_TIMER1        BIT(4)
#define IRQ_TIMER2        BIT(5)
#define IRQ_TIMER3        BIT(6)
#define IRQ_IPC_SYNC      BIT(16)
#define IRQ_IPC_SEND      BIT(17)
#define IRQ_IPC_RECV      BIT(18)

/* ===================================================================== */
/* Keypad — GBATEK DS I/O map, docs/hardware_notes.md §11                 */
/* ===================================================================== */
#define KEYINPUT         0x04000130u   /* 16-bit, active-low (both cores)  */
#define KEYCNT           0x04000132u   /* 16-bit, key IRQ control          */
#define EXTKEYIN         0x04000136u   /* 16-bit, ARM7 (X/Y/pen/hinge)     */

/* KEYINPUT bits. Buttons are active-low: 0 = pressed. */
#define KEY_A            BIT(0)
#define KEY_B            BIT(1)
#define KEY_SELECT       BIT(2)
#define KEY_START        BIT(3)
#define KEY_RIGHT        BIT(4)
#define KEY_LEFT         BIT(5)
#define KEY_UP           BIT(6)
#define KEY_DOWN         BIT(7)
#define KEY_R            BIT(8)
#define KEY_L            BIT(9)

/* EXTKEYIN bits (ARM7-only extension keys), also active-low. */
#define EXTKEY_X         BIT(0)
#define EXTKEY_Y         BIT(1)
#define EXTKEY_PEN       BIT(6)   /* stylus down (DSi)                      */
#define EXTKEY_HINGE     BIT(7)   /* lid closed                             */

/* ===================================================================== */
/* IPC bridge ARM9 <-> ARM7 — docs/hardware_notes.md §3                   */
/* ===================================================================== */
/*
 * Widths (GBATEK DS I/O map):
 *   0x04000180  IPCSYNC     16-bit
 *   0x04000184  IPCFIFOCNT  16-bit
 *   0x04000188  IPCFIFOSEND 32-bit
 *   0x04100000  IPCFIFORECV 32-bit
 */
#define IPC_SYNC         0x04000180u   /* 16-bit                           */
#define IPC_FIFOCNT      0x04000184u   /* 16-bit                           */
#define IPC_FIFOSEND     0x04000188u   /* 32-bit                           */
#define IPC_FIFORECV     0x04100000u   /* 32-bit                           */

/*
 * Bit layout (GBATEK "IPC Fifo Control Register"): status bits are read-only
 * and *core-relative* — bit 0 reports this CPU's send FIFO, bit 8 its receive
 * FIFO.
 *
 *   bit  0  R    Send FIFO empty      (1 = empty)
 *   bit  1  R    Send FIFO full       (1 = full)
 *   bit  2  R/W  Send FIFO empty IRQ enable
 *   bit  3  W    Send FIFO clear
 *   bit  8  R    Receive FIFO empty   (1 = empty)
 *   bit  9  R    Receive FIFO full    (1 = full)
 *   bit 10  R/W  Receive FIFO not-empty IRQ enable
 *   bit 14  R/W  Error (read-on-empty / send-on-full)
 *   bit 15  R/W  FIFO master enable
 *
 * NOTE: an earlier revision defined SEND_EMPTY as bit 1 and SEND_FULL as
 * bit 2. That is the *full* and *send-empty-IRQ* bits, so every send-empty
 * poll (arm9_ipc_wait_send_empty / ipc_fifo_wait_empty) actually waited for
 * the FIFO to become full and hung forever on the first IPC transaction.
 */
#define IPC_FIFOCNT_SEND_EMPTY   BIT(0)   /* 1 = send FIFO empty (R)      */
#define IPC_FIFOCNT_SEND_FULL    BIT(1)   /* 1 = send FIFO full (R)       */
#define IPC_FIFOCNT_SEND_IRQ     BIT(2)   /* send-empty IRQ enable (R/W)  */
#define IPC_FIFOCNT_SEND_CLEAR   BIT(3)   /* write 1 to flush send FIFO   */
#define IPC_FIFOCNT_RECV_EMPTY   BIT(8)   /* 1 = receive FIFO empty (R)   */
#define IPC_FIFOCNT_RECV_FULL    BIT(9)   /* 1 = receive FIFO full (R)    */
#define IPC_FIFOCNT_RECV_IRQ     BIT(10)  /* recv-not-empty IRQ (R/W)     */
#define IPC_FIFOCNT_ERROR        BIT(14)  /* read-empty / send-full error */
#define IPC_FIFOCNT_ENABLE       BIT(15)  /* FIFO master enable           */

/* Bring the FIFO up: master enable + receive-not-empty IRQ. */
#define IPC_FIFOCNT_INIT_VALUE \
    (IPC_FIFOCNT_ENABLE | IPC_FIFOCNT_RECV_IRQ)

/* Pin the status-bit positions so the historical shift-by-one cannot return. */
DSI_STATIC_ASSERT(IPC_FIFOCNT_SEND_EMPTY == 0x0001u, ipc_fifocnt_send_empty_bit0);
DSI_STATIC_ASSERT(IPC_FIFOCNT_SEND_FULL  == 0x0002u, ipc_fifocnt_send_full_bit1);
DSI_STATIC_ASSERT(IPC_FIFOCNT_RECV_EMPTY == 0x0100u, ipc_fifocnt_recv_empty_bit8);
DSI_STATIC_ASSERT(IPC_FIFOCNT_ENABLE     == 0x8000u, ipc_fifocnt_enable_bit15);

/* ===================================================================== */
/* Timers — docs/hardware_notes.md §2                                    */
/* ===================================================================== */
#define TIMER_BASE       0x04000100u
#define TIMER0_DATA      (TIMER_BASE + 0x00u)
#define TIMER0_CNT       (TIMER_BASE + 0x02u)
#define TIMER1_DATA      (TIMER_BASE + 0x04u)
#define TIMER1_CNT       (TIMER_BASE + 0x06u)
#define TIMER2_DATA      (TIMER_BASE + 0x08u)
#define TIMER2_CNT       (TIMER_BASE + 0x0Au)
#define TIMER3_DATA      (TIMER_BASE + 0x0Cu)
#define TIMER3_CNT       (TIMER_BASE + 0x0Eu)

#define TIMER_CNT_ENABLE     BIT(7)
#define TIMER_CNT_IRQ        BIT(6)
#define TIMER_CNT_PRESCALER  (3u << 0)

/* ===================================================================== */
/* SPI bus — docs/hardware_notes.md §5                                    */
/* ===================================================================== */
/* SPICNT and SPIDATA are 16-bit; a 32-bit store covers both at once. */
#define SPI_CNT          0x040001C0u   /* 16-bit                           */
#define SPI_DATA         0x040001C2u   /* 16-bit (low byte is the datum)   */
#define SPI_CNT_BUSY     BIT(7)
#define SPI_CNT_ENABLE   BIT(15)

/* AUXSPICNT is 16-bit; AUXSPIDATA is 16-bit with an 8-bit datum. */
#define AUXSPI_CNT       0x040001A0u   /* 16-bit                           */
#define AUXSPI_DATA      0x040001A2u   /* 16-bit                           */

/* ===================================================================== */
/* RTC / PMIC bit-bang transport — docs/hardware_notes.md §6              */
/* ===================================================================== */
#define RTC_TRANSFER     0x04000138u   /* 8-bit serial bus                 */

/* ===================================================================== */
/* Small convenience: halt the CPU until an interrupt occurs (ARM7 SWI 2) */
/* ===================================================================== */
#if defined(__arm__) || defined(__thumb__)
static inline void dsi_halt(void)
{
    __asm__ volatile ("swi 0x02" ::: "memory");
}
#else
static inline void dsi_halt(void) { }
#endif

#endif /* DSI_COMMON_DSI_HW_H */
