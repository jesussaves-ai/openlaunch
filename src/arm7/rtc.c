/*
 * src/arm7/rtc.c
 *
 * Bit-banged RTC/PMIC transport at 0x04000138.
 * See include/arm7/rtc.h and docs/hardware_notes.md §6.
 *
 * Protocol reconstructed from FUN_0001cf1c/40/54/60:
 *   - 0x73 selects/idles the bus.
 *   - A data bit is sent as (bit + base) with clock low, then the same value
 *     OR 2 released with clock high. `base` is 0x74 (FUN_0001cf54) or
 *     0x64 (FUN_0001cf60).
 *   - The input bit is sampled from bit 0 during the clock-high phase.
 */
#include "arm7/rtc.h"
#include "common/dsi_hw.h"

/* The reference spaces edges with `SWI 3`; a short bounded spin is equivalent
 * for boot-time use and avoids depending on the BIOS. */
static void rtc_edge_delay(void)
{
    volatile u32 spin = 32u;
    while (spin != 0u) {
        spin--;
    }
}

/* @ref FUN_0001cf40  (address 0x01CF40) — idle/select. */
void rtc_select(void)
{
    REG8(RTC_TRANSFER) = 0x73u;
}

/* @ref FUN_0001cf1c  (address 0x01CF1C) — select then register latch. */
void rtc_deselect(void)
{
    REG8(RTC_TRANSFER) = 0x73u;
    rtc_edge_delay();
    REG8(RTC_TRANSFER) = 0x77u;
}

/*
 * Shift one byte out (LSB first) and return the byte shifted in (LSB-first in
 * time), matching FUN_0001cf54 / FUN_0001cf60.
 */
static u8 rtc_shift_byte(u8 value, u8 bit_base)
{
    u8 in = 0u;
    u8 i;

    for (i = 0u; i < 8u; i++) {
        REG8(RTC_TRANSFER) = (u8)((value & 1u) + bit_base);
        rtc_edge_delay();
        REG8(RTC_TRANSFER) = (u8)(REG8(RTC_TRANSFER) | 2u);
        rtc_edge_delay();

        /* Capture MISO (bit 0) into the output byte, LSB first. */
        in = (u8)(in | ((REG8(RTC_TRANSFER) & 1u) << i));
        value = (u8)(value >> 1);
    }
    return in;
}

void rtc_write_byte(u8 addr, bool small_bit_base)
{
    (void)rtc_shift_byte(addr, small_bit_base ? 0x64u : 0x74u);
}

/*
 * @ref FUN_0001cfb0  (address 0x01CFB0)
 *   Emits a header byte (address << 4 | 0x86) and then `len` payload bytes.
 *
 * NOTE: the reference routine actually *reads* into memory (the shift helper
 * returns the MISO bits). The write variant uses 0x06 as the command suffix
 * (see FUN_0001cfdc); rtc_read_burst() below is the read counterpart.
 */
void rtc_write_burst(u32 header, const u8 *data, u32 len)
{
    u32 i;

    rtc_select();
    (void)rtc_shift_byte((u8)((header << 4) | 0x06u), 0x74u);

    for (i = 0u; i < len; i++) {
        (void)rtc_shift_byte(data[i], 0x64u);
    }
    rtc_deselect();
}

u8 rtc_bcd_decode(u8 value)
{
    return (u8)(((value >> 4) & 0x0Fu) * 10u + (value & 0x0Fu));
}

/* @ref FUN_0001cfb0 (0x01CFB0) — read `len` bytes from command register `cmd`. */
void rtc_read_burst(u8 cmd, u8 *dst, u32 len)
{
    u32 i;

    rtc_select();
    /* Command byte: fixed 0b0110 pattern, read bit 7, command in bits 4-6. */
    (void)rtc_shift_byte((u8)(((u32)cmd << 4) | 0x86u), 0x74u);

    for (i = 0u; i < len; i++) {
        dst[i] = rtc_shift_byte(0x00u, 0x64u);
    }
    rtc_deselect();
}

bool rtc_read_datetime(rtc_datetime_t *out)
{
    u8 buf[7];
    u8 status = 0u;

    if (out == NULL) {
        return false;
    }

    /* Command 2 = date & time (year, month, day, weekday, hh, mm, ss). */
    rtc_read_burst(2u, buf, 7u);

    out->year = rtc_bcd_decode(buf[0]);
    out->month = rtc_bcd_decode((u8)(buf[1] & 0x1Fu));
    out->day = rtc_bcd_decode((u8)(buf[2] & 0x3Fu));
    out->weekday = (u8)(buf[3] & 0x07u);
    out->hour = rtc_bcd_decode((u8)(buf[4] & 0x3Fu));
    out->minute = rtc_bcd_decode((u8)(buf[5] & 0x7Fu));
    out->second = rtc_bcd_decode((u8)(buf[6] & 0x7Fu));

    /* Status register 1 (power flags). */
    rtc_read_burst(0u, &status, 1u);
    out->status = status;

    /* Range sanity: reject a floating bus / corrupt BCD. */
    if (out->month < 1u || out->month > 12u) {
        return false;
    }
    if (out->day < 1u || out->day > 31u) {
        return false;
    }
    if (out->hour > 23u || out->minute > 59u || out->second > 59u) {
        return false;
    }
    return true;
}
