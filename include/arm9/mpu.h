/*
 * include/arm9/mpu.h
 *
 * ARM946E-S protection unit and TCM configuration.
 *
 * @ref FUN_0602c374 (address 0x0602C374), called from the ARM9 entry stub. It
 * programs CP15 c2 (cacheable / translation table base), c3 (domain access),
 * c5 (access permissions), c6 (protection regions), c9 (DTCM/ITCM) and finally
 * sets the CP15 control register to 0x41005 (MPU + I/D cache + ITCM enable).
 * The raw region descriptors are read from the stub literal pool; decoded
 * base/size values are recorded in docs/hardware_notes.md §ARM9 MPU.
 */
#ifndef DSI_ARM9_MPU_H
#define DSI_ARM9_MPU_H

#include "../common/types.h"

/* Data protection regions exposed by the ARM946E-S (CP15 c6, c0..c7). */
#define ARM9_MPU_REGIONS        8u

/* Interrupt/alias windows described by the reference descriptor set. */
#define ARM9_MPU_BASE           (0x06020000u)   /* reconstructed code image */

/* CP15 control bits set by the reference stub (control |= ARM9_CP15_ENABLE). */
#define ARM9_CP15_ENABLE        0x00041005u
/* Boot-time CP15 control value captured from the reference crt0 stub. */
#define ARM9_CP15_BOOT_CONTROL  0x00057078u

/*
 * Pure decoders for a CP15 c6 region descriptor.
 *
 *   bits[31:12] = region base address
 *   bits[5:1]   = size field N, region size = 2^(N+1) bytes
 *   bit[0]      = region enable
 */
u32  arm9_mpu_region_base(u32 descriptor);
u32  arm9_mpu_region_size(u32 descriptor);
bool arm9_mpu_region_enabled(u32 descriptor);

/*
 * Program the complete reference protection-unit state:
 *   - disable the MPU (CP15 c1 bit 0)
 *   - set c2/c3 translation base + domain access
 *   - write c5 data/instruction access permissions
 *   - write the c6 data-region descriptors
 *   - program DTCM/ITCM via c9
 *   - re-enable MPU + I/D cache + ITCM
 *
 * On non-ARM hosts (unit tests) this compiles to a no-op.
 */
void arm9_mpu_configure(void);

/* Enable just the MPU/cache/ITCM bits in CP15 control (control |= 0x41005). */
void arm9_mpu_enable(void);

/*
 * Read the current CP15 control register (c1,c0,0). Returns 0 on non-ARM
 * hosts so the menu's register dump stays host-compilable.
 */
u32 arm9_mpu_read_control(void);

#endif /* DSI_ARM9_MPU_H */
