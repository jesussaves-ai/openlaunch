/*
 * src/common/nds_header.c
 *
 * Minimal NDS/DSi header reader. See include/common/nds_header.h.
 *
 * The CRC algorithm matches scripts/pack_nds.py / scripts/verify_rom.py:
 * seed 0xFFFF, reflected polynomial 0xA001.
 */
#include "common/nds_header.h"
#include "common/mem.h"

u16 nds_crc16(const u8 *data, u32 len)
{
    u32 crc = 0xFFFFu;
    u32 i;
    u32 bit;

    if (data == NULL) {
        return 0u;
    }

    for (i = 0u; i < len; i++) {
        crc ^= (u32)data[i];
        for (bit = 0u; bit < 8u; bit++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1) ^ 0xA001u;
            } else {
                crc >>= 1;
            }
        }
    }
    return (u16)(crc & 0xFFFFu);
}

static char printable(char c)
{
    u8 u = (u8)c;

    return (u >= 0x20u && u < 0x7Fu) ? c : ' ';
}

bool nds_header_parse(const u8 *buf, u32 len, nds_header_t *out)
{
    u32 i;

    if (buf == NULL || out == NULL || len < NDS_HEADER_MIN_LEN) {
        return false;
    }

    dsi_memset(out, 0, sizeof(*out));

    for (i = 0u; i < NDS_TITLE_LEN; i++) {
        out->title[i] = printable((char)buf[NDS_TITLE_OFF + i]);
    }
    out->title[NDS_TITLE_LEN] = '\0';
    /* Trim trailing padding spaces. */
    while (i > 0u && out->title[i - 1u] == ' ') {
        out->title[i - 1u] = '\0';
        i--;
    }

    for (i = 0u; i < NDS_GAMECODE_LEN; i++) {
        out->gamecode[i] = printable((char)buf[NDS_GAMECODE_OFF + i]);
    }
    out->gamecode[NDS_GAMECODE_LEN] = '\0';

    out->stored_crc = dsi_read_le16(buf + NDS_HEADER_CRC_OFF);
    out->computed_crc = nds_crc16(buf, NDS_HEADER_CRC_LEN);
    out->crc_ok = (out->stored_crc == out->computed_crc);

    return true;
}
