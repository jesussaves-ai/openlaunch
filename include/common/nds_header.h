/*
 * include/common/nds_header.h
 *
 * Minimal NDS/DSi (.nds / .srl) header reader.
 *
 * Only the fields needed to identify a bootable image are exposed: the title,
 * the 4-character game code, and the header CRC16 (offset 0x15E, covering
 * 0x000..0x15D with the Modbus/ARC polynomial 0xA001).
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

typedef struct {
    char title[NDS_TITLE_LEN + 1u];       /* trimmed, NUL-terminated        */
    char gamecode[NDS_GAMECODE_LEN + 1u]; /* NUL-terminated                 */
    u8   unit_code;                       /* raw 0x012 unit code            */
    bool twl_capable;                     /* unit code advertises DSi support */
    u16  stored_crc;
    u16  computed_crc;
    bool crc_ok;
} nds_header_t;

/* Modbus/ARC CRC16: seed 0xFFFF, polynomial 0xA001 (LSB-first). */
u16 nds_crc16(const u8 *data, u32 len);

/*
 * Parse and validate a header from `buf` (`len` readable bytes). Requires at
 * least NDS_HEADER_MIN_LEN bytes. Non-printable title/gamecode bytes are
 * replaced so the result is always safe to render.
 */
bool nds_header_parse(const u8 *buf, u32 len, nds_header_t *out);

#endif /* DSI_COMMON_NDS_HEADER_H */
