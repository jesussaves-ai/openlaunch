/*
 * src/common/dldi.c
 *
 * Clean-room DLDI section scanner and in-RAM patcher. See
 * include/common/dldi.h for the format description and the safety contract.
 *
 * The relocation algorithm mirrors Chishm's documented patch process (the
 * "dlditool" behaviour): the driver is copied over the target stub, its header
 * address table and IO function pointers are shifted by the destination delta,
 * any still-unrelocated pointer inside the data/glue/GOT ranges is shifted when
 * the matching FIX_* flag is set, and the BSS range is zero-filled. Everything
 * is done with explicit little-endian byte accesses so the code is correct on
 * the host tests as well as on the little-endian ARM targets.
 */
#include "common/dldi.h"
#include "common/mem.h"

/* The 12-byte signature literal: 0xBF8DA5ED + " Chishm\0". */
static const u8 s_dldi_signature[DLDI_MAGIC_LEN] = {
    0xEDu, 0xA5u, 0x8Du, 0xBFu,
    ' ', 'C', 'h', 'i', 's', 'h', 'm', 0x00u
};

/* ASCII-printable guard for decoded strings. */
static char dldi_printable(u8 c)
{
    return (c >= 0x20u && c < 0x7Fu) ? (char)c : '.';
}

bool dldi_find_offset(const u8 *arm9_buf, u32 arm9_len, u32 *out_offset)
{
    u32 off;

    if (arm9_buf == NULL) {
        return false;
    }

    /*
     * Scan on 4-byte boundaries, requiring room for a whole header (not just
     * the 12-byte signature) after the match so a stray signature in the last
     * few bytes of the image cannot be mistaken for a DLDI section.
     */
    for (off = 0u; off + DLDI_HEADER_SIZE <= arm9_len; off += 4u) {
        if (dsi_memcmp(arm9_buf + off, s_dldi_signature,
                       DLDI_MAGIC_LEN) == 0) {
            if (out_offset != NULL) {
                *out_offset = off;
            }
            return true;
        }
    }
    return false;
}

bool dldi_parse_header(const u8 *buf, u32 len, dldi_info_t *out)
{
    dldi_info_t info;
    u32 alloc_log2;
    u32 drv_log2;
    u32 i;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
    }
    if (buf == NULL || len < DLDI_HEADER_SIZE) {
        return false;
    }
    if (dsi_memcmp(buf + DLDI_OFF_MAGIC, s_dldi_signature,
                   DLDI_MAGIC_LEN) != 0) {
        return false;
    }

    alloc_log2 = buf[DLDI_OFF_ALLOC_SIZE];
    drv_log2 = buf[DLDI_OFF_DRIVER_SIZE];
    if (alloc_log2 < DLDI_ALLOC_LOG2_MIN || alloc_log2 > DLDI_ALLOC_LOG2_MAX) {
        return false;
    }
    /* Keep the size arithmetic well-defined (1u << 16 is the largest sane). */
    if (drv_log2 > DLDI_ALLOC_LOG2_MAX) {
        return false;
    }

    dsi_memset(&info, 0, sizeof(info));
    info.version = buf[DLDI_OFF_VERSION];
    info.driver_size_log2 = (u8)drv_log2;
    info.fix_flags = buf[DLDI_OFF_FIX_FLAGS];
    info.allocated_size_log2 = (u8)alloc_log2;
    info.driver_size = 1u << drv_log2;
    info.allocated_size = 1u << alloc_log2;

    /* The reserved block must fit inside the buffer we were handed. */
    if (info.allocated_size > len) {
        return false;
    }

    /* Friendly name: copy up to 48 bytes, stop at NUL, sanitise. */
    for (i = 0u; i < DLDI_NAME_LEN; i++) {
        u8 c = buf[DLDI_OFF_NAME + i];

        if (c == 0x00u) {
            break;
        }
        info.friendly_name[i] = dldi_printable(c);
    }
    info.friendly_name[i] = '\0';

    for (i = 0u; i < DLDI_IO_TYPE_LEN; i++) {
        info.io_type[i] = dldi_printable(buf[DLDI_OFF_IO_TYPE + i]);
    }
    info.io_type[DLDI_IO_TYPE_LEN] = '\0';

    info.data_start      = dsi_read_le32(buf + DLDI_OFF_DATA_START);
    info.data_end        = dsi_read_le32(buf + DLDI_OFF_DATA_END);
    info.glue_start      = dsi_read_le32(buf + DLDI_OFF_GLUE_START);
    info.glue_end        = dsi_read_le32(buf + DLDI_OFF_GLUE_END);
    info.got_start       = dsi_read_le32(buf + DLDI_OFF_GOT_START);
    info.got_end         = dsi_read_le32(buf + DLDI_OFF_GOT_END);
    info.bss_start       = dsi_read_le32(buf + DLDI_OFF_BSS_START);
    info.bss_end         = dsi_read_le32(buf + DLDI_OFF_BSS_END);
    info.fn_startup      = dsi_read_le32(buf + DLDI_OFF_STARTUP);
    info.fn_is_inserted  = dsi_read_le32(buf + DLDI_OFF_IS_INSERTED);
    info.fn_read_sectors = dsi_read_le32(buf + DLDI_OFF_READ);
    info.fn_write_sectors= dsi_read_le32(buf + DLDI_OFF_WRITE);
    info.fn_clear_status = dsi_read_le32(buf + DLDI_OFF_CLEAR);
    info.fn_shutdown     = dsi_read_le32(buf + DLDI_OFF_SHUTDOWN);

    if (out != NULL) {
        /*
         * Explicit call avoids a libc memcpy dependency (GCC can lower a large
         * struct assignment to memcpy even under -ffreestanding).
         */
        dsi_memcpy(out, &info, sizeof(info));
    }
    return true;
}

bool dldi_scan(const u8 *buf, u32 len, dldi_info_t *out, u32 *out_offset)
{
    u32 off;

    if (!dldi_find_offset(buf, len, &off)) {
        if (out != NULL) {
            dsi_memset(out, 0, sizeof(*out));
        }
        return false;
    }
    if (!dldi_parse_header(buf + off, len - off, out)) {
        return false;
    }
    if (out_offset != NULL) {
        *out_offset = off;
    }
    return true;
}

/* Add `delta` to the 32-bit little-endian field at `field_off`. */
static void dldi_relocate_field(u8 *base, u32 field_off, u32 delta)
{
    dsi_write_le32(base + field_off,
                   dsi_read_le32(base + field_off) + delta);
}

/*
 * Walk the byte range [range_start-ddmem_start, range_end-ddmem_start) of the
 * copied driver. Any 32-bit word that still points into the driver's original
 * address window [ddmem_start, ddmem_start+ddmem_size) is relocated by `delta`.
 * Stops before reading past `copy_len`.
 */
static void dldi_fix_range(u8 *base, u32 copy_len, u32 ddmem_start,
                           u32 range_start, u32 range_end,
                           u32 ddmem_size, u32 delta)
{
    u32 ddmem_end;
    u32 start_off;
    u32 end_off;
    u32 i;

    if (range_end < range_start || range_start < ddmem_start) {
        return;
    }

    ddmem_end = ddmem_start + ddmem_size;
    start_off = range_start - ddmem_start;
    end_off = range_end - ddmem_start;

    for (i = start_off; i < end_off; i++) {
        u32 v;

        /* A 32-bit load must fit entirely inside the copied region. */
        if (i > copy_len || copy_len - i < 4u) {
            break;
        }
        v = dsi_read_le32(base + i);
        if (v >= ddmem_start && v < ddmem_end) {
            dsi_write_le32(base + i, v + delta);
        }
    }
}

/* Zero-fill [bss_start, bss_end) of the copied driver (clamped to copy_len). */
static void dldi_zero_bss(u8 *base, u32 copy_len, u32 ddmem_start,
                          u32 bss_start, u32 bss_end)
{
    u32 off;
    u32 len;

    if (bss_end <= bss_start || bss_start < ddmem_start) {
        return;
    }
    off = bss_start - ddmem_start;
    len = bss_end - bss_start;
    if (off >= copy_len) {
        return;
    }
    if (len > copy_len - off) {
        len = copy_len - off;
    }
    dsi_memset(base + off, 0, len);
}

bool dldi_apply_patch(u8 *arm9_buf, u32 arm9_len, u32 arm9_ram_base,
                      const u8 *driver_buf, u32 driver_len)
{
    u32 off;
    dldi_info_t tgt;
    dldi_info_t drv;
    u32 target_data_start;
    u32 delta;
    u32 copy_len;

    if (arm9_buf == NULL || driver_buf == NULL) {
        return false;
    }
    if (driver_len < DLDI_HEADER_SIZE) {
        return false;
    }

    if (!dldi_find_offset(arm9_buf, arm9_len, &off)) {
        return false;
    }
    if (!dldi_parse_header(arm9_buf + off, arm9_len - off, &tgt)) {
        return false;
    }
    if (!dldi_parse_header(driver_buf, driver_len, &drv)) {
        return false;
    }

    /* The replacement must fit the space the target reserved. */
    if (drv.driver_size_log2 > tgt.allocated_size_log2) {
        return false;
    }
    if (driver_len > drv.driver_size || driver_len > tgt.allocated_size ||
        driver_len > arm9_len - off) {
        return false;
    }

    /*
     * The stub header begins at arm9_buf + off, which is loaded at
     * arm9_ram_base + off; that is the destination the driver's link-time
     * addresses must be rebased to.
     */
    target_data_start = arm9_ram_base + off;
    delta = target_data_start - drv.data_start;
    copy_len = driver_len;

    dsi_memcpy(arm9_buf + off, driver_buf, copy_len);

    /* Keep the target's reserved-space declaration in the patched header. */
    arm9_buf[off + DLDI_OFF_ALLOC_SIZE] = tgt.allocated_size_log2;

    /* Relocate the eight address-table entries... */
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_DATA_START, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_DATA_END, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_GLUE_START, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_GLUE_END, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_GOT_START, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_GOT_END, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_BSS_START, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_BSS_END, delta);

    /* ...and the six IO_INTERFACE function pointers. */
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_STARTUP, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_IS_INSERTED, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_READ, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_WRITE, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_CLEAR, delta);
    dldi_relocate_field(arm9_buf + off, DLDI_OFF_SHUTDOWN, delta);

    /*
     * The section scans use the driver's original bounds; the header fields
     * were already rebased above, so they now fall outside the driver window
     * and are skipped by the range guard (exactly as dlditool intends).
     */
    if ((drv.fix_flags & DLDI_FIX_ALL) != 0u) {
        dldi_fix_range(arm9_buf + off, copy_len, drv.data_start,
                       drv.data_start, drv.data_end,
                       drv.driver_size, delta);
    }
    if ((drv.fix_flags & DLDI_FIX_GLUE) != 0u) {
        dldi_fix_range(arm9_buf + off, copy_len, drv.data_start,
                       drv.glue_start, drv.glue_end,
                       drv.driver_size, delta);
    }
    if ((drv.fix_flags & DLDI_FIX_GOT) != 0u) {
        dldi_fix_range(arm9_buf + off, copy_len, drv.data_start,
                       drv.got_start, drv.got_end,
                       drv.driver_size, delta);
    }
    if ((drv.fix_flags & DLDI_FIX_BSS) != 0u) {
        dldi_zero_bss(arm9_buf + off, copy_len, drv.data_start,
                      drv.bss_start, drv.bss_end);
    }

    return true;
}

/* ---- Built-in read-only stub driver ----------------------------------- */

/* Store a little-endian word inside the built-in driver image. */
static void dldi_ro_put32(u8 *p, u32 off, u32 value)
{
    dsi_write_le32(p + off, value);
}

/* Copy a NUL-terminated string into the 48-byte friendly-name field. */
static void dldi_ro_put_name(u8 *p, const char *s)
{
    u32 i = 0u;

    while (s[i] != '\0' && i < DLDI_NAME_LEN) {
        p[DLDI_OFF_NAME + i] = (u8)s[i];
        i++;
    }
}

/* Write one ARM "mov r0,#imm ; bx lr" stub at `off`. */
static void dldi_ro_put_arm_stub(u8 *p, u32 off, u32 imm)
{
    dldi_ro_put32(p, off, 0xE3A00000u | (imm & 0xFFu)); /* mov r0, #imm */
    dldi_ro_put32(p, off + 4u, 0xE12FFF1Eu);             /* bx  lr       */
}

const u8 *dldi_builtin_ro_driver(void)
{
    static u8 drv[DLDI_BUILTIN_RO_SIZE];
    static bool ready = false;

    if (!ready) {
        dsi_memset(drv, 0, sizeof(drv));

        dsi_write_le32(drv + DLDI_OFF_MAGIC, DLDI_MAGIC);
        drv[DLDI_OFF_STRING + 0u] = ' ';
        drv[DLDI_OFF_STRING + 1u] = 'C';
        drv[DLDI_OFF_STRING + 2u] = 'h';
        drv[DLDI_OFF_STRING + 3u] = 'i';
        drv[DLDI_OFF_STRING + 4u] = 's';
        drv[DLDI_OFF_STRING + 5u] = 'h';
        drv[DLDI_OFF_STRING + 6u] = 'm';
        drv[DLDI_OFF_STRING + 7u] = 0x00u;

        drv[DLDI_OFF_VERSION] = DLDI_VERSION_VALUE;
        drv[DLDI_OFF_DRIVER_SIZE] = 10u;              /* 1 KiB .dldi + BSS */
        drv[DLDI_OFF_FIX_FLAGS] = DLDI_FIX_BSS;
        drv[DLDI_OFF_ALLOC_SIZE] = 10u;               /* 1 KiB reserved    */

        dldi_ro_put_name(drv, "DSiR Read-Only Stub");

        dldi_ro_put32(drv, DLDI_OFF_DATA_START, DLDI_BUILTIN_RO_BASE);
        dldi_ro_put32(drv, DLDI_OFF_DATA_END,   DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_GLUE_START, DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_GLUE_END,   DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_GOT_START,  DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_GOT_END,    DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_BSS_START,  DLDI_BUILTIN_RO_BASE + 0x3C0u);
        dldi_ro_put32(drv, DLDI_OFF_BSS_END,    DLDI_BUILTIN_RO_BASE + 0x400u);

        drv[DLDI_OFF_IO_TYPE + 0u] = 'D';
        drv[DLDI_OFF_IO_TYPE + 1u] = 'S';
        drv[DLDI_OFF_IO_TYPE + 2u] = 'i';
        drv[DLDI_OFF_IO_TYPE + 3u] = 'R';

        dldi_ro_put32(drv, DLDI_OFF_FEATURES, 0x00000001u); /* CANREAD */

        dldi_ro_put32(drv, DLDI_OFF_STARTUP,       DLDI_BUILTIN_RO_BASE + 0x100u);
        dldi_ro_put32(drv, DLDI_OFF_IS_INSERTED,   DLDI_BUILTIN_RO_BASE + 0x110u);
        dldi_ro_put32(drv, DLDI_OFF_READ,          DLDI_BUILTIN_RO_BASE + 0x120u);
        dldi_ro_put32(drv, DLDI_OFF_WRITE,         DLDI_BUILTIN_RO_BASE + 0x130u);
        dldi_ro_put32(drv, DLDI_OFF_CLEAR,         DLDI_BUILTIN_RO_BASE + 0x140u);
        dldi_ro_put32(drv, DLDI_OFF_SHUTDOWN,      DLDI_BUILTIN_RO_BASE + 0x150u);

        dldi_ro_put_arm_stub(drv, 0x100u, 1u);   /* startup:      true  */
        dldi_ro_put_arm_stub(drv, 0x110u, 1u);   /* is_inserted:  true  */
        dldi_ro_put_arm_stub(drv, 0x120u, 0u);   /* read:         false */
        dldi_ro_put_arm_stub(drv, 0x130u, 0u);   /* write:        false */
        dldi_ro_put_arm_stub(drv, 0x140u, 1u);   /* clear_status: true  */
        dldi_ro_put_arm_stub(drv, 0x150u, 1u);   /* shutdown:     true  */

        ready = true;
    }

    return drv;
}
