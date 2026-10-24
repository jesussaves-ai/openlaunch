/*
 * include/common/arm7_handoff.h
 *
 * Addresses used by the boot-stage-2 ARM7 takeover ("jumpslide").
 *
 * The TMD overflow leaves the ARM9 executing our code while the ARM7 keeps
 * running the system firmware. Stage 2's own IPCSYNC title-boot handshake can
 * no longer reach that ARM7 (it is past its boot path), so the ARM9 hijacks
 * the task dispatcher instead:
 *
 *   1. the overflow bootstrap stages our ARM7 image at ARM7_HANDOFF_ENTRY and
 *      sets ARM7_HANDOFF_MARKER_ADDR to ARM7_HANDOFF_MARKER_MAGIC;
 *   2. the ARM9 takes NWRAM-C part 4 (the firmware ARM7's 12-byte-stride task
 *      table) through MBK5/MBK8, fills the whole 32 KiB with
 *      ARM7_HANDOFF_ENTRY, and hands the part back to the ARM7;
 *   3. the next task dispatch (0x037D4D94 `blx r2`) branches to our entry.
 *
 * The MBK values below are the ones official Unlaunch uses at 0x06025590; see
 * docs/stage2_re.md. The marker and entry live in the Main-RAM window at
 * 0x0237Fxxx that is free at the exploit.
 *
 * Keep the .equ values in src/arm9/overflow_bootstrap.s in sync with this file.
 */
#ifndef DSI_COMMON_ARM7_HANDOFF_H
#define DSI_COMMON_ARM7_HANDOFF_H

#include "types.h"

/* Set by the overflow bootstrap once the ARM7 image has been staged. */
#define ARM7_HANDOFF_MARKER_ADDR   0x0237FFF0u
#define ARM7_HANDOFF_MARKER_MAGIC  0x54424C4Fu  /* "OLBT" */

/* Written by our ARM7 once it is executing (diagnostic / GDB check). */
#define ARM7_READY_MARKER_ADDR     0x0237FFF4u
#define ARM7_READY_MARKER_MAGIC    0x59444152u  /* "RADY" */

/* Written by the ARM7 reset stub before any stack/BSS setup. */
#define ARM7_JUMP_MARKER_ADDR      0x0237FFE8u
#define ARM7_JUMP_MARKER_MAGIC     0x44504D4Au  /* "JMPD" */

/* Our ARM7 image: linked at 0x02380000 by ld/arm7.ld. */
#define ARM7_HANDOFF_ENTRY         0x02380000u

/* Firmware ARM7 return/stack window (ARM9 view after the NWRAM-C remap). */
#define ARM7_HANDOFF_SLIDE_ADDR    0x03803040u
#define ARM7_HANDOFF_SLIDE_WORDS   0x800u

/*
 * MBK values copied from Astronaut's stage-2 takeover:
 *   MBK8 = WRAM-C window at 0x03800000, 256 KiB;
 *   MBK5 = enable WRAM-C slot 4 for the ARM9.
 * MBK5/MBK8 are restored to their stage-2 values afterwards.
 */
#define ARM7_HANDOFF_MBK8_WINDOW   0x0C003800u
#define ARM7_HANDOFF_MBK5_SLOT     0x00000080u

/*
 * Stage 2's ARM7 dispatches tasks through a 12-byte-per-entry table that lives
 * in NWRAM-C part 4 (ARM7 address 0x037F0000-0x037F7FFF with the stock window
 * 0x037B8000-0x037F8000). The ARM9 can take that part over with MBK5, fill the
 * whole 32 KiB with one address, and hand it back: the next task dispatch then
 * branches to that address. This is what official Unlaunch does at 0x06025590
 * (it fills 0x06023CD4 and then ring 7); see docs/stage2_re.md.
 */
#define ARM7_HANDOFF_TASK_WINDOW_ADDR  0x03800000u
#define ARM7_HANDOFF_TASK_WINDOW_SIZE  0x00008000u
/* Ownership byte for NWRAM-C part 4: ARM9 slot 0, then ARM7 slot 6. */
#define ARM7_HANDOFF_PART4_ARM9        0x00000080u
#define ARM7_HANDOFF_PART4_ARM7        0x00000099u

#endif /* DSI_COMMON_ARM7_HANDOFF_H */
