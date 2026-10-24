/*
 * include/arm9/arm7_boot.h
 *
 * ARM7 hand-off for the DSi boot-stage-2 overflow boot.
 *
 * Boot stage 2 normally starts the ARM7 as part of booting a title, but the
 * TMD-overflow exploit makes stage 2 jump straight into OpenLaunch, so the
 * ARM7 keeps running the system firmware. Two mechanisms live here:
 *
 *   arm9_boot_arm7()
 *     The reference hand-off: publish a 352-byte NDS header at 0x02FFFE00 with
 *     the ARM7 entry at offset 0x34 and ring IPCSYNC. This only works while the
 *     ARM7 is still in its boot flow (i.e. when stage 2 is booting a title);
 *     from the overflow it is a no-op. Kept for the title-boot path and for
 *     reference, gated behind `make ARM7_HANDOFF=1`.
 *
 *   arm9_arm7_task_table_fill()
 *     The overflow hand-off that works: stage 2's ARM7 dispatches tasks
 *     through a table in NWRAM-C part 4. The ARM9 takes the part over, fills
 *     the 32 KiB table with our ARM7 entry and hands it back; the next
 *     dispatch branches to us. Same mechanism as official Unlaunch
 *     (0x06025590). See docs/stage2_re.md.
 *
 *   arm9_arm7_takeover()
 *     An earlier overflow hand-off: hijack the *running* ARM7 with a NWRAM-C
 *     remap and a return-address jumpslide (Astronaut, GPL-3.0-or-later). It
 *     targets the wrong window on this firmware and is disabled by default;
 *     kept for reference/experiments.
 */
#ifndef DSI_ARM9_ARM7_BOOT_H
#define DSI_ARM9_ARM7_BOOT_H

#include "../common/types.h"
#include "../common/arm7_handoff.h"

/*
 * Publish the reference boot header and signal the ARM7 over IPCSYNC. Bounded
 * (never blocks forever). Only meaningful when the ARM7 is still in its boot
 * flow; see the module comment above.
 */
void arm9_boot_arm7(void);

/*
 * Take over the running firmware ARM7 from the TMD-overflow path.
 *
 * Runs only when the overflow bootstrap left ARM7_HANDOFF_MARKER_MAGIC at
 * ARM7_HANDOFF_MARKER_ADDR (i.e. our ARM7 image is staged). The temporary
 * MBK5/MBK8 remap is fully undone; the caller then decides the post-takeover
 * NWRAM configuration (arm9_main() keeps stage 2's WRAM-C ownership so the
 * ARM7 retains its IWRAM/stacks). Must be called with IRQs disabled, before
 * scfg_init() remaps NWRAM.
 *
 * Returns true when the jumpslide was armed, false when the marker is absent,
 * the console is not in TWL mode, or WRAM-C is write-protected.
 */
bool arm9_arm7_takeover(void);

/*
 * Redirect the firmware ARM7's task dispatcher (TMD-overflow path).
 *
 * Stage 2's ARM7 calls tasks through a table in NWRAM-C part 4. The ARM9 maps
 * that part to its own 0x03800000 window (MBK8), takes ownership (MBK5 byte 0),
 * fills the 32 KiB with `entry`, flushes the data cache and hands the part
 * back to the ARM7 (slot 6). The next dispatch then branches to `entry`.
 *
 * Mirrors official Unlaunch's routine at 0x06025590, which fills
 * 0x06023CD4 and rings IPCSYNC 7. Requires TWL mode; returns false otherwise.
 * Leaves the ARM9's NWRAM-C window at 0x03800000 (as Unlaunch does).
 */
bool arm9_arm7_task_table_fill(u32 entry);

#endif /* DSI_ARM9_ARM7_BOOT_H */
