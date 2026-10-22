/*
 * include/common/nds_header.h
 *
 * Minimal NDS/DSi (.nds / .srl) header reader and loadability validator.
 *
 * The reader exposes the fields needed to identify a bootable image (title and
 * 4-character game code), the header CRC16 (offset 0x15E, covering 0x000..0x15D
 * with the Modbus/ARC polynomial 0xA001), and the ARM9/ARM7 load descriptors
 * used by the chainloader.
 *
 * nds_header_validate_loadable() is the pure safety gate run before any RAM is
 * touched: it checks the CRC16 and proves that the ARM9 and ARM7 load ranges
 * are 4-byte aligned, non-zero, inside safe Main RAM (or ARM7 WRAM) and clear
 * of the bootloader's own active reservation.
 */
#ifndef DSI_COMMON_NDS_HEADER_H
#define DSI_COMMON_NDS_HEADER_H

#include "types.h"

#define NDS_TITLE_OFF       0x000u
#define NDS_TITLE_LEN       12u
#define NDS_GAMECODE_OFF    0x00Cu
#define NDS_GAMECODE_LEN    4u
#define NDS_UNIT_CODE_OFF   0x012u
#define NDS_HEADER_CRC_OFF  0x15Eu
#define NDS_HEADER_CRC_LEN  0x15Eu   /* bytes covered by the header CRC     */
#define NDS_HEADER_MIN_LEN  0x160u   /* bytes needed to read/verify the CRC */
#define NDS_HEADER_SECTOR_SIZE 512u  /* sector 0 of an .nds image           */

/*
 * Standard NDS header ARM9/ARM7 load descriptor offsets (GBATEK "NDS Header").
 * The values are byte offsets into the 512-byte sector-0 header.
 */
#define NDS_ARM9_ROM_OFFSET_OFF 0x020u
#define NDS_ARM9_ENTRY_OFF      0x024u
#define NDS_ARM9_RAM_ADDR_OFF   0x028u
#define NDS_ARM9_SIZE_OFF       0x02Cu
#define NDS_ARM7_ROM_OFFSET_OFF 0x030u
#define NDS_ARM7_ENTRY_OFF      0x034u
#define NDS_ARM7_RAM_ADDR_OFF   0x038u
#define NDS_ARM7_SIZE_OFF       0x03Cu

/*
 * Cartridge unit code (offset 0x012). This tells a DSi launcher / emulator
 * whether the image is a plain NDS title or DSi-capable; a DSi-capable image
 * is launched with TWL mode active (SCFG_A9ROM == 0x01).
 *
 *   0x00  NDS only
 *   0x02  NDS + DSi (hybrid): runs on both, DSi leaves TWL mode on
 *   0x03  DSi-exclusive
 *
 * We build the hybrid (0x02) so melonDS/Unlaunch keep native TWL mode while a
 * legacy DS can still run the same image.
 */
#define NDS_UNIT_CODE_NDS       0x00u
#define NDS_UNIT_CODE_NDS_DSI   0x02u
#define NDS_UNIT_CODE_DSI       0x03u
#define NDS_UNIT_CODE_TWL_MASK  0x02u   /* bit1 set => DSi-capable            */

/* ---- Safe-RAM policy for the chainloader ------------------------------ */

/* Inclusive base and exclusive end of the Main-RAM window an image may use. */
#define NDS_SAFE_RAM_BASE       0x02000000u
#define NDS_SAFE_RAM_END_DS     0x023BFE00u   /* 4 MiB DS window, minus slack */
#define NDS_SAFE_RAM_END_TWL    0x02FE0000u   /* 16 MiB TWL window, minus slack */

/* ARM7-private WRAM window a new ARM7 image may be copied into. */
#define NDS_ARM7_WRAM_BASE      0x037F8000u
#define NDS_ARM7_WRAM_END       0x0380FFC0u   /* just below the ARM7 stacks  */

/*
 * The bootloader's own active reservation: the code image, its .data/.bss and
 * the shared bounce buffer used to stage an ARM7 segment before the handoff.
 * A new ARM9/ARM7 load range must not intersect [BASE, END).
 *
 * The bounce buffer sits directly below the linked image; the image itself is
 * linked at 0x02200000 (ld/arm9.ld) and 0x02380000 (ld/arm7.ld). Reserving a
 * full 512 KiB window keeps head-room without colliding with the canonical
 * homebrew load addresses (ARM9 0x02000000, ARM7 0x02380000).
 */
#define NDS_BOOTLOADER_RESERVE_BASE 0x02100000u
#define NDS_BOOTLOADER_RESERVE_END  0x02280000u

/*
 * Canonical RAM address the firmware/BIOS expects the 512-byte NDS header copy
 * at. The DS address is a mirror of 0x023FFE00; the DSi one is the top of the
 * 16 MiB window.
 */
#define NDS_HEADER_COPY_ADDR_DS   0x027FFE00u
#define NDS_HEADER_COPY_ADDR_TWL  0x02FFFE00u

typedef struct {
    char title[NDS_TITLE_LEN + 1u];       /* trimmed, NUL-terminated        */
    char gamecode[NDS_GAMECODE_LEN + 1u]; /* NUL-terminated                 */
    u8   unit_code;                       /* raw 0x012 unit code            */
    bool twl_capable;                     /* unit code advertises DSi support */
    u16  stored_crc;
    u16  computed_crc;
    bool crc_ok;

    /* ARM9 load descriptor (0x020..0x02F). */
    u32  arm9_rom_offset;   /* file offset of the ARM9 binary              */
    u32  arm9_entry;        /* ARM9 entry point (RAM)                      */
    u32  arm9_ram_addr;     /* ARM9 load address (RAM)                     */
    u32  arm9_size;         /* ARM9 binary size in bytes                   */

    /* ARM7 load descriptor (0x030..0x03F). */
    u32  arm7_rom_offset;   /* file offset of the ARM7 binary              */
    u32  arm7_entry;        /* ARM7 entry point (RAM)                      */
    u32  arm7_ram_addr;     /* ARM7 load address (RAM)                     */
    u32  arm7_size;         /* ARM7 binary size in bytes                   */
} nds_header_t;

/*
 * Loadability verdict. NDS_LOAD_OK == 0 so the value can be used directly as a
 * boolean "false means OK" and still read naturally in tests.
 */
typedef enum {
    NDS_LOAD_OK = 0,
    NDS_ERR_BAD_CRC,            /* header CRC16 mismatch                    */
    NDS_ERR_ARM9_BOUNDS,        /* ARM9 offset/size/entry invalid/out of RAM */
    NDS_ERR_ARM7_BOUNDS,        /* ARM7 offset/size/entry invalid/out of RAM */
    NDS_ERR_OVERLAP_BOOTLOADER, /* range intersects the loader reservation   */
    NDS_ERR_FILE_TRUNCATED      /* header too short or payload past EOF      */
} nds_load_status_t;

/*
 * Host-testable safety window. `ram_end` / `arm7_wram_end` are exclusive; the
 * reservation is [reserve_base, reserve_end).
 */
typedef struct {
    u32 ram_base;
    u32 ram_end;
    u32 arm7_wram_base;
    u32 arm7_wram_end;
    u32 reserve_base;
    u32 reserve_end;
} nds_load_bounds_t;

/* Modbus/ARC CRC16: seed 0xFFFF, polynomial 0xA001 (LSB-first). */
u16 nds_crc16(const u8 *data, u32 len);

/*
 * Parse and validate a header from `buf` (`len` readable bytes). Requires at
 * least NDS_HEADER_MIN_LEN bytes. Non-printable title/gamecode bytes are
 * replaced so the result is always safe to render.
 */
bool nds_header_parse(const u8 *buf, u32 len, nds_header_t *out);

/* Fill `out` with the platform default safety window (`extended_ram` selects
 * the 4 MiB DS or the 16 MiB TWL Main-RAM limit). */
void nds_load_bounds_default(nds_load_bounds_t *out, bool extended_ram);

/*
 * Pure validation gate for the chainloader.
 *
 * `buf`/`len`: the 512-byte sector-0 header (len >= NDS_HEADER_SECTOR_SIZE).
 * `file_size`: total size of the .nds file (0 = unknown -> truncation checks
 * are skipped).
 * `bounds`: safety window; NULL selects the DS defaults.
 * `out` (optional): receives the fully parsed header on success (and on the
 * specific error returns below, when parsing itself succeeded).
 *
 * Returns NDS_LOAD_OK, or one of the specific NDS_ERR_* codes.
 */
nds_load_status_t nds_header_validate_loadable(const u8 *buf, u32 len,
                                               u32 file_size,
                                               const nds_load_bounds_t *bounds,
                                               nds_header_t *out);

/* Human-readable text for a load status (never NULL). */
const char *nds_load_status_text(nds_load_status_t status);

#endif /* DSI_COMMON_NDS_HEADER_H */
