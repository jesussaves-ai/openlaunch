/*
 * src/arm7/spi.c
 *
 * ARM7 SPI bus. See include/arm7/spi.h and docs/hardware_notes.md §5.
 */
#include "arm7/spi.h"
#include "arm7/irq.h"
#include "common/dsi_hw.h"

/* Device SPICNT control words observed in the reference image. */
#define SPICNT_CTRL_PMIC  0x8002u   /* FUN_0001c96c (DAT_0001ce7c)         */
#define SPICNT_CTRL_DEV1  0x8201u   /* FUN_0001c978 (DAT_0001ce80)         */
#define SPICNT_CTRL_FIRM  0x8100u   /* FUN_0001c984                         */
#define SPICNT_CTRL_AUX   0x8900u   /* FUN_0001c984 alternate               */
/* Firmware select with chip-select hold (bit 11) for multi-byte commands. */
#define SPICNT_CTRL_FIRM_HOLD 0x8900u

/*
 * @ref FUN_0001c96c  (address 0x01C96C)
 *
 * The 32-bit store at 0x040001C0 writes SPICNT in the low half and SPIDATA in
 * the high half, then bit 7 of SPICNT is polled until the transfer completes.
 */
u8 spi_xfer(u32 ctrl, u32 data, bool restore_irq)
{
    u32 saved;
    u8 result;

    saved = irq_disable_all();   /* reference clears IME during the transfer */

    REG32(SPI_CNT) = ctrl | (data << 16);

    while ((REG32(SPI_CNT) & SPI_CNT_BUSY) != 0u) {
        /* poll */
    }

    result = (u8)(REG16(SPI_DATA) & 0xFFu);

    if (restore_irq) {
        irq_restore(saved);
    }
    return result;
}

/*
 * @ref FUN_0001c9d8 / FUN_0001c9f4
 *   PMIC/touchscreen byte exchanges (data 0x00 / 0x80 respectively).
 */
u8 spi_pmic_write(u32 ctrl, u32 data, bool restore_irq)
{
    return spi_xfer(ctrl, data, restore_irq);
}

/*
 * @ref FUN_0001ca10 / FUN_0001ca30  (addresses 0x01CA10 / 0x01CA30)
 *   Firmware read: send the address MSB-first (4 bytes), returning the last
 *   byte shifted in.
 */
u8 spi_firmware_read(u32 address)
{
    u32 shift;
    u8 last = 0u;

    for (shift = 24u; ; shift -= 8u) {
        last = spi_xfer(SPICNT_CTRL_FIRM, (address >> shift) & 0xFFu, false);
        if (shift == 0u) {
            break;
        }
    }
    return last;
}

/*
 * JEDEC ID read (0x9F) from the firmware chip. The chip-select hold bit keeps
 * the device selected across the four bytes; it is released afterwards by
 * clearing SPICNT.
 */
bool spi_firmware_id(u32 *out)
{
    u32 saved;
    u8 manuf;
    u8 type;
    u8 cap;

    if (out == NULL) {
        return false;
    }

    saved = irq_disable_all();

    manuf = spi_xfer(SPICNT_CTRL_FIRM_HOLD, 0x9Fu, false);
    type = spi_xfer(SPICNT_CTRL_FIRM_HOLD, 0x00u, false);
    cap = spi_xfer(SPICNT_CTRL_FIRM_HOLD, 0x00u, false);

    REG16(SPI_CNT) = 0u;   /* release chip select */

    irq_restore(saved);

    *out = ((u32)manuf << 16) | ((u32)type << 8) | (u32)cap;

    /* A floating bus reads back all-zero or all-one. */
    if (manuf == 0xFFu && type == 0xFFu && cap == 0xFFu) {
        return false;
    }
    if (manuf == 0x00u && type == 0x00u && cap == 0x00u) {
        return false;
    }
    return true;
}

/*
 * Standard SPI flash "Read Data" (0x03): chip-select held low, command byte,
 * then a 24-bit address shifted out MSB-first, then `len` bytes clocked back.
 * The chip-select hold bit (0x0800) keeps the device selected across the whole
 * transaction; clearing SPICNT afterwards releases it.
 *
 * This mirrors the multi-byte framing already used by spi_firmware_id().
 */
bool spi_firmware_read_buf(u32 address, void *dst, u32 len)
{
    u8 *out = (u8 *)dst;
    u32 saved;
    u32 i;

    if (dst == NULL || len == 0u) {
        return false;
    }

    saved = irq_disable_all();

    (void)spi_xfer(SPICNT_CTRL_FIRM_HOLD, 0x03u, false);            /* READ */
    (void)spi_xfer(SPICNT_CTRL_FIRM_HOLD, (address >> 16) & 0xFFu, false);
    (void)spi_xfer(SPICNT_CTRL_FIRM_HOLD, (address >> 8) & 0xFFu, false);
    (void)spi_xfer(SPICNT_CTRL_FIRM_HOLD, address & 0xFFu, false);

    for (i = 0u; i < len; i++) {
        out[i] = spi_xfer(SPICNT_CTRL_FIRM_HOLD, 0x00u, false);
    }

    REG16(SPI_CNT) = 0u;   /* release chip select */

    irq_restore(saved);
    return true;
}
