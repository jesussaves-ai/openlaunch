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
 *   arm9_arm7_takeover()
 *     The overflow hand-off: hijack the *running* firmware ARM7 with a NWRAM-C
 *     remap and a return-address jumpslide. The overflow bootstrap stages our
 *     ARM7 image at 0x02380000 and sets a marker; this function is marker-gated
 *     so the normal `.nds` build never touches the running ARM7. Adapted from
 *     Astronaut (GPL-3.0-or-later); see docs/stage2_re.md.
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

#endif /* DSI_ARM9_ARM7_BOOT_H */
