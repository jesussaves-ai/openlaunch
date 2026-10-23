/*
 * include/arm9/arm7_boot.h
 *
 * ARM7 hand-off for the DSi boot-stage-2 overflow boot.
 *
 * Boot stage 2 normally performs this as part of booting a title, but the
 * TMD-overflow exploit makes stage 2 jump straight into OpenLaunch instead, so
 * stage 2 never runs it and the ARM7 keeps running the system firmware. This
 * module reproduces the hand-off so OpenLaunch's own ARM7 can be started.
 *
 * Mechanism (recovered from the decrypted stage 2, see docs/stage2_re.md):
 *   ARM9 helper 0x037B9AC4 : wait for IPCSYNC == 6, then write 0x600.
 *   ARM7 0x02380970/0x0238091C : signals IPCSYNC, reads the boot header at
 *                                0x02FFFE00 and takes the ARM7 entry from
 *                                header offset 0x34.
 *
 * So: publish a valid 352-byte NDS header (ARM7 entry at 0x34) at 0x02FFFE00,
 * then ring IPCSYNC.
 */
#ifndef DSI_ARM9_ARM7_BOOT_H
#define DSI_ARM9_ARM7_BOOT_H

#include "../common/types.h"

/* Publish the boot header and signal the ARM7 over IPCSYNC. Bounded (never
 * blocks forever); safe to call once after the ARM7 image is staged. */
void arm9_boot_arm7(void);

#endif /* DSI_ARM9_ARM7_BOOT_H */
