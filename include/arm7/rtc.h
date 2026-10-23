/*
 * include/arm7/rtc.h
 *
 * Bit-banged RTC / PMIC transport at 0x04000138.
 *
 * Derived from FUN_0001cf1c, FUN_0001cf40, FUN_0001cf54 and FUN_0001cf60
 * (see docs/hardware_notes.md §6).
 *
 * SAFETY (the project conventions): the same serial transport reaches the BPTWL power
 * management IC. Only the documented RTC/PMIC registers may be written; never
 * probe undocumented addresses.
 */
#ifndef DSI_ARM7_RTC_H
#define DSI_ARM7_RTC_H

#include "../common/types.h"

/*
 * Select the RTC transport and place it in the idle state (writes 0x73).
 *
 * @ref FUN_0001cf40  (address 0x01CF40)
 */
void rtc_select(void);

/*
 * Release / deselect the transport after a transaction.
 *
 * @ref FUN_0001cf1c  (address 0x01CF1C)
 */
void rtc_deselect(void);

/*
 * Shift one byte out of the transport, LSB first.
 * @param addr  command/register address byte
 *
 * @ref FUN_0001cf54  (address 0x01CF54) uses base bit value 0x74,
 * @ref FUN_0001cf60  (address 0x01CF60) uses 0x64.
 */
void rtc_write_byte(u8 addr, bool small_bit_base);

/*
 * Write a buffer to the transport.
 *
 * @ref FUN_0001cfb0  (address 0x01CFB0) — chip/register header then payload.
 * @ref FUN_0001cfdc  (address 0x01CFDC) — raw payload variant.
 */
void rtc_write_burst(u32 header, const u8 *data, u32 len);

/* ------------------------------------------------------------------ */
/* Read path (S-35180 / S-35199A01 command set)                        */
/* ------------------------------------------------------------------ */

/*
 * Decoded date/time returned by rtc_read_datetime().
 *
 * The command set and register layout are documented in GBATEK "DS Real-Time
 * Clock (RTC)": the read command byte is (cmd << 4) | 0x86 (LSB-first), and
 * command 2 returns year, month, day, weekday, hour, minute, second (all BCD).
 */
typedef struct {
    u8 year;      /* 0..99  (20xx)      */
    u8 month;     /* 1..12              */
    u8 day;       /* 1..31              */
    u8 weekday;   /* 0..6               */
    u8 hour;      /* 0..23              */
    u8 minute;    /* 0..59              */
    u8 second;    /* 0..59              */
    u8 status;    /* status register 1  */
} rtc_datetime_t;

/*
 * Read `len` parameter bytes from RTC command register `cmd`.
 * @ref FUN_0001cfb0  (address 0x01CFB0) — command 0x86 | (cmd << 4), then
 *      `len` bytes shifted in with bit base 0x64.
 */
void rtc_read_burst(u8 cmd, u8 *dst, u32 len);

/*
 * Read the current date and time. Returns true when the values pass a range
 * sanity check; false when the RTC is unreadable or the BCD is corrupt.
 */
bool rtc_read_datetime(rtc_datetime_t *out);

/* Decode one packed BCD byte (e.g. 0x23 -> 23). Pure / host-testable. */
u8 rtc_bcd_decode(u8 value);

#endif /* DSI_ARM7_RTC_H */
