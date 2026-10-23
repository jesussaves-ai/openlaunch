/*
 * include/arm7/spi.h
 *
 * ARM7 SPI / AUXSPI bus access.
 *
 * Derived from FUN_0001c96c, FUN_0001c978, FUN_0001c984, FUN_0001c9d8,
 * FUN_0001c9f4, FUN_0001ca10 and FUN_0001ca30 (see docs/hardware_notes.md §5).
 */
#ifndef DSI_ARM7_SPI_H
#define DSI_ARM7_SPI_H

#include "../common/types.h"

/* On-chip devices selected through the chip-select bits in SPICNT[19:16]. */
typedef enum {
    SPI_CS_PMIC   = 0,   /* BPTWL power management / touchscreen           */
    SPI_CS_FIRM   = 1,   /* firmware flash (0x8100 base ctrl value)        */
    SPI_CS_TOUCH  = 2,   /* touchscreen controller                         */
    SPI_CS_UNUSED = 3
} spi_chip_t;

/*
 * Perform one byte transfer while holding the caller's IRQ state.
 *
 * @ref FUN_0001c96c / FUN_0001c978 / FUN_0001c984
 *   r0 = data byte
 *   r1 = device SPICNT control word (enable + chip-select + baudrate)
 *   r2 = 1 to keep IRQs enabled (restore IME), otherwise IRQs stay masked
 *   return: byte shifted out of SPIDATA (0x040001C2)
 *
 * The reference performs a single 32-bit write to 0x040001C0 which covers
 * SPICNT (low half) and SPIDATA (high half): SPICNT = ctrl | (data << 16).
 * It then polls SPICNT bit 7 (busy) until clear.
 *
 * @param ctrl         device control word. Reference values:
 *                       0x8002  (FUN_0001c96c, PMIC/touchscreen)
 *                       0x8201  (FUN_0001c978)
 *                       0x8100 / 0x8900 (FUN_0001c984, firmware)
 *                       0x840C2B13 observed near the firmware path
 * @param data         byte to transmit
 * @param restore_irq  true => restore IME afterwards
 */
u8 spi_xfer(u32 ctrl, u32 data, bool restore_irq);

/*
 * Send a PMIC/touchscreen byte (chip-select 0, IRQs restored).
 *
 * @ref FUN_0001c9d8 / FUN_0001c9f4
 */
u8 spi_pmic_write(u32 ctrl, u32 data, bool restore_irq);

/*
 * Read the firmware ID / a 24-bit firmware address (chip-select 1).
 *
 * @ref FUN_0001ca10  (address 0x01CA10)
 *   Shifts a little-endian address out MSB first by repeatedly calling the
 *   firmware SPICNT variant.
 */
u8 spi_firmware_read(u32 address);

/*
 * Read the JEDEC flash identification (SPI command 0x9F) from the firmware
 * chip. On success `*out` holds (manufacturer << 16) | (type << 8) | capacity.
 * Returns false when the bus floats (all 0x00 / 0xFF), i.e. no firmware chip
 * is present in the current console mode.
 */
bool spi_firmware_id(u32 *out);

/*
 * Read `len` bytes from the firmware flash starting at the 24-bit `address`
 * into `dst`, using the standard SPI "Read Data" command (0x03) with the
 * chip-select held across the whole transaction.
 *
 * This is the ARM7-side hardware primitive behind IPC_CMD_READ_FIRMWARE
 * (architectural rule: the ARM9 must not drive SPI itself).
 * Read-only; it never issues a program/erase command.
 */
bool spi_firmware_read_buf(u32 address, void *dst, u32 len);

#endif /* DSI_ARM7_SPI_H */
