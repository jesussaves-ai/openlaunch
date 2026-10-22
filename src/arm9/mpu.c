/*
 * src/arm9/mpu.c
 *
 * ARM946E-S protection unit / TCM setup. See include/arm9/mpu.h.
 *
 * @ref FUN_0602c374 (address 0x0602C374).
 *
 * The reference literal pool at 0x0602C424..0x0602C450 holds the exact CP15
 * operands; they are reproduced verbatim below so the region map matches the
 * original image. Decoding (docs/hardware_notes.md §ARM9 MPU):
 *
 *   region 0  0x00000000  +128 MiB   (whole low address space)
 *   region 1  0x02000000  +  4 MiB   (Main RAM, NDS window)
 *   region 2  0x06000000  + 16 MiB   (VRAM banks)
 *   region 3  0x08000000  +128 MiB   (GBA/legacy window)
 *   region 4  0x03000000  + 16 KiB   (Shared WRAM)
 *   region 5  0x01000000  + 16 MiB   (ARM9 code / ITCM window)
 *   region 6  0xFFFF0000  + 32 KiB   (high vectors)
 *   region 7  0x02C00000  +  4 MiB   (extended Main RAM tail)
 */
#include "arm9/mpu.h"

/* ------------------------------------------------------------------ */
/* Pure descriptor decoders (host-testable).                           */
/* ------------------------------------------------------------------ */

u32 arm9_mpu_region_base(u32 descriptor)
{
    return descriptor & 0xFFFFF000u;
}

u32 arm9_mpu_region_size(u32 descriptor)
{
    u32 field = (descriptor >> 1) & 0x1Fu;

    /* 2^(field + 1); field max is 31 on the ARM946E-S, so cap at 2^32. */
    if (field >= 31u) {
        return 0xFFFFFFFFu;
    }
    return 1u << (field + 1u);
}

bool arm9_mpu_region_enabled(u32 descriptor)
{
    return (descriptor & 1u) != 0u;
}

/* ------------------------------------------------------------------ */
/* Hardware programming.                                               */
/* ------------------------------------------------------------------ */

#if defined(__arm__) || defined(__thumb__)

/* Raw CP15 operand values, copied from the stub literal pool. */
#define REF_C2_TTB          0x00000046u   /* cacheable / TTB base          */
#define REF_C3_DAC          0x00000006u   /* domain access control         */
#define REF_C5_DATA         0x15111111u   /* data access permissions       */
#define REF_C5_INSTR        0x05100111u   /* instruction access permissions*/
#define REF_DTCM            0x0300000Au   /* c9,c1,0                       */
#define REF_ITCM            0x00000020u   /* c9,c1,1                       */

typedef struct {
    u8  region;   /* CRm of CP15 c6 (0..7) */
    u32 value;    /* descriptor            */
} arm9_mpu_region_write_t;

static const arm9_mpu_region_write_t region_writes[] = {
    { 0u, 0x00000035u },
    { 1u, 0x0200002Bu },
    { 3u, 0x08000035u },
    { 4u, 0x0300001Bu },
    { 6u, 0xFFFF001Du },
    { 7u, 0x02C0002Bu },
    { 2u, 0x00000000u },   /* region 2 temporarily disabled      */
    { 5u, 0x0100002Fu },
    { 2u, 0x0600002Fu },   /* region 2 re-armed for VRAM         */
};

static inline u32 cp15_read_control(void)
{
    u32 value;
    __asm__ volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(value));
    return value;
}

static inline void cp15_write_control(u32 value)
{
    __asm__ volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r"(value) : "memory");
}

static inline void cp15_write_c2(u32 value)
{
    __asm__ volatile ("mcr p15, 0, %0, c2, c0, 0" :: "r"(value) : "memory");
    __asm__ volatile ("mcr p15, 0, %0, c2, c0, 1" :: "r"(value) : "memory");
}

static inline void cp15_write_c3(u32 value)
{
    __asm__ volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r"(value) : "memory");
}

static inline void cp15_write_c5(u32 value, u32 opcode2)
{
    if (opcode2 == 3u) {
        __asm__ volatile ("mcr p15, 0, %0, c5, c0, 3" :: "r"(value) : "memory");
    } else {
        __asm__ volatile ("mcr p15, 0, %0, c5, c0, 2" :: "r"(value) : "memory");
    }
}

static inline void cp15_write_region(u32 index, u32 value)
{
    switch (index) {
    case 0u: __asm__ volatile ("mcr p15, 0, %0, c6, c0, 0" :: "r"(value) : "memory"); break;
    case 1u: __asm__ volatile ("mcr p15, 0, %0, c6, c1, 0" :: "r"(value) : "memory"); break;
    case 2u: __asm__ volatile ("mcr p15, 0, %0, c6, c2, 0" :: "r"(value) : "memory"); break;
    case 3u: __asm__ volatile ("mcr p15, 0, %0, c6, c3, 0" :: "r"(value) : "memory"); break;
    case 4u: __asm__ volatile ("mcr p15, 0, %0, c6, c4, 0" :: "r"(value) : "memory"); break;
    case 5u: __asm__ volatile ("mcr p15, 0, %0, c6, c5, 0" :: "r"(value) : "memory"); break;
    case 6u: __asm__ volatile ("mcr p15, 0, %0, c6, c6, 0" :: "r"(value) : "memory"); break;
    case 7u: __asm__ volatile ("mcr p15, 0, %0, c6, c7, 0" :: "r"(value) : "memory"); break;
    default: break;
    }
}

static inline void cp15_write_tcm(u32 value, u32 opcode2)
{
    if (opcode2 == 0u) {
        __asm__ volatile ("mcr p15, 0, %0, c9, c1, 0" :: "r"(value) : "memory");
    } else {
        __asm__ volatile ("mcr p15, 0, %0, c9, c1, 1" :: "r"(value) : "memory");
    }
}

void arm9_mpu_configure(void)
{
    u32 control = cp15_read_control();

    /* 1. Disable the MPU before changing region state. */
    cp15_write_control(control & 0xFFFFFFFEu);

    cp15_write_c2(REF_C2_TTB);
    cp15_write_c3(REF_C3_DAC);
    cp15_write_c5(REF_C5_DATA, 2u);
    cp15_write_c5(REF_C5_INSTR, 3u);

    for (u32 i = 0; i < ARRAY_SIZE(region_writes); i++) {
        cp15_write_region(region_writes[i].region, region_writes[i].value);
    }

    cp15_write_tcm(REF_DTCM, 0u);
    cp15_write_tcm(REF_ITCM, 1u);

    arm9_mpu_enable();
}

void arm9_mpu_enable(void)
{
    cp15_write_control(cp15_read_control() | ARM9_CP15_ENABLE);
}

u32 arm9_mpu_read_control(void)
{
    return cp15_read_control();
}

#else /* !ARM — host build: nothing to do. */

void arm9_mpu_configure(void) { }
void arm9_mpu_enable(void)    { }
u32  arm9_mpu_read_control(void) { return 0u; }

#endif
