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
#define NDS_HEADER_CRC_OFF  0x15Eu
#define NDS_HEADER_CRC_LEN  0x15Eu   /* bytes covered by the header CRC     */
#define NDS_HEADER_MIN_LEN  0x160u   /* bytes needed to read/verify the CRC */

typedef struct {
    char title[NDS_TITLE_LEN + 1u];       /* trimmed, NUL-terminated        */
    char gamecode[NDS_GAMECODE_LEN + 1u]; /* NUL-terminated                 */
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
