/*
 * src/common/nds_header.c
 *
 * NDS/DSi header reader and chainloader safety gate.
 * See include/common/nds_header.h.
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

    out->unit_code = buf[NDS_UNIT_CODE_OFF];
    out->twl_capable = (out->unit_code & NDS_UNIT_CODE_TWL_MASK) != 0u;

    out->stored_crc = dsi_read_le16(buf + NDS_HEADER_CRC_OFF);
    out->computed_crc = nds_crc16(buf, NDS_HEADER_CRC_LEN);
    out->crc_ok = (out->stored_crc == out->computed_crc);

    out->arm9_rom_offset = dsi_read_le32(buf + NDS_ARM9_ROM_OFFSET_OFF);
    out->arm9_entry      = dsi_read_le32(buf + NDS_ARM9_ENTRY_OFF);
    out->arm9_ram_addr   = dsi_read_le32(buf + NDS_ARM9_RAM_ADDR_OFF);
    out->arm9_size       = dsi_read_le32(buf + NDS_ARM9_SIZE_OFF);

    out->arm7_rom_offset = dsi_read_le32(buf + NDS_ARM7_ROM_OFFSET_OFF);
    out->arm7_entry      = dsi_read_le32(buf + NDS_ARM7_ENTRY_OFF);
    out->arm7_ram_addr   = dsi_read_le32(buf + NDS_ARM7_RAM_ADDR_OFF);
    out->arm7_size       = dsi_read_le32(buf + NDS_ARM7_SIZE_OFF);

    /*
     * The TWL extended header is only meaningful for a DSi-capable image and
     * only readable when the caller supplied enough bytes. Decoding it is
     * purely additive: a DS-only (0x00) or short header leaves it zeroed.
     */
    if (out->twl_capable && len >= NDS_HEADER_TWL_MIN_LEN) {
        out->twl_flags         = buf[NDS_TWL_FLAGS_OFF];
        out->arm9i_rom_offset  = dsi_read_le32(buf + NDS_TWL_ARM9I_ROM_OFF);
        out->arm9i_ram_address = dsi_read_le32(buf + NDS_TWL_ARM9I_RAM_OFF);
        out->arm9i_size        = dsi_read_le32(buf + NDS_TWL_ARM9I_SIZE_OFF);
        out->arm7i_rom_offset  = dsi_read_le32(buf + NDS_TWL_ARM7I_ROM_OFF);
        out->arm7i_ram_address = dsi_read_le32(buf + NDS_TWL_ARM7I_RAM_OFF);
        out->arm7i_size        = dsi_read_le32(buf + NDS_TWL_ARM7I_SIZE_OFF);
        out->twl_header_valid  = true;
    }

    return true;
}

bool nds_header_read(const u8 *buf, u32 len, nds_header_t *out)
{
    return nds_header_parse(buf, len, out);
}

/* ------------------------------------------------------------------ */
/* Safety window                                                       */
/* ------------------------------------------------------------------ */

void nds_load_bounds_default(nds_load_bounds_t *out, bool extended_ram)
{
    if (out == NULL) {
        return;
    }

    out->extended_ram   = extended_ram;
    out->ram_base       = NDS_SAFE_RAM_BASE;
    out->ram_end        = extended_ram ? NDS_SAFE_RAM_END_TWL
                                       : NDS_SAFE_RAM_END_DS;
    out->arm7_wram_base = NDS_ARM7_WRAM_BASE;
    out->arm7_wram_end  = NDS_ARM7_WRAM_END;
    out->nwram_base     = NDS_NWRAM_BASE;
    out->nwram_end      = NDS_NWRAM_END;
    out->reserve_base   = NDS_BOOTLOADER_RESERVE_BASE;
    out->reserve_end    = NDS_BOOTLOADER_RESERVE_END;
}

/* True when value is non-zero and 4-byte aligned. */
static bool aligned_nonzero(u32 value)
{
    return value != 0u && (value & 3u) == 0u;
}

/* True when [base, base+size) lies inside [lo, hi). Overflow-safe. */
static bool range_within(u32 base, u32 size, u32 lo, u32 hi)
{
    if (size == 0u || hi <= lo) {
        return false;
    }
    if (base < lo || size > hi - lo) {
        return false;
    }
    return (base - lo) <= (hi - lo) - size;
}

/* True when [base, base+size) intersects [lo, hi). Overflow-safe. */
static bool range_overlaps(u32 base, u32 size, u32 lo, u32 hi)
{
    if (size == 0u || hi <= lo) {
        return false;
    }
    if (base >= hi) {
        return false;             /* starts at/after the window            */
    }
    if (base < lo) {
        return size > lo - base;  /* base < lo: does it extend past lo?    */
    }
    return true;                  /* base is inside [lo, hi)               */
}

/* True when `value` is inside [base, base+size). */
static bool addr_in_range(u32 value, u32 base, u32 size)
{
    return value >= base && (value - base) < size;
}

/* True when the file range [off, off+size) is inside a file of `file_size`. */
static bool file_range_ok(u32 off, u32 size, u32 file_size)
{
    return off <= file_size && size <= file_size - off;
}

nds_load_status_t nds_header_validate_loadable(const u8 *buf, u32 len,
                                               u32 file_size,
                                               const nds_load_bounds_t *bounds,
                                               nds_header_t *out)
{
    nds_header_t h;
    nds_load_bounds_t def;

    if (bounds == NULL) {
        nds_load_bounds_default(&def, false);
        bounds = &def;
    }

    /* The loader must be able to read a complete sector-0 header. */
    if (buf == NULL || len < NDS_HEADER_SECTOR_SIZE) {
        return NDS_ERR_FILE_TRUNCATED;
    }
    if (!nds_header_parse(buf, len, &h)) {
        return NDS_ERR_FILE_TRUNCATED;
    }
    if (out != NULL) {
        /* Explicit call avoids a libc memcpy dependency (GCC can lower a
         * large struct assignment to memcpy even under -ffreestanding). */
        dsi_memcpy(out, &h, sizeof(h));
    }

    if (!h.crc_ok) {
        return NDS_ERR_BAD_CRC;
    }

    /* --- Descriptor well-formedness ---------------------------------- */
    if (!aligned_nonzero(h.arm9_rom_offset) ||
        !aligned_nonzero(h.arm9_ram_addr) ||
        !aligned_nonzero(h.arm9_entry) ||
        !aligned_nonzero(h.arm9_size)) {
        return NDS_ERR_ARM9_BOUNDS;
    }
    if (!aligned_nonzero(h.arm7_rom_offset) ||
        !aligned_nonzero(h.arm7_ram_addr) ||
        !aligned_nonzero(h.arm7_entry) ||
        !aligned_nonzero(h.arm7_size)) {
        return NDS_ERR_ARM7_BOUNDS;
    }

    /* --- The payload must actually exist in the file ----------------- */
    if (file_size != 0u) {
        if (file_size < NDS_HEADER_SECTOR_SIZE ||
            !file_range_ok(h.arm9_rom_offset, h.arm9_size, file_size) ||
            !file_range_ok(h.arm7_rom_offset, h.arm7_size, file_size)) {
            return NDS_ERR_FILE_TRUNCATED;
        }
    }

    /* --- ARM9 target must be safely inside Main RAM ------------------ */
    if (!range_within(h.arm9_ram_addr, h.arm9_size,
                      bounds->ram_base, bounds->ram_end) ||
        !addr_in_range(h.arm9_entry, h.arm9_ram_addr, h.arm9_size)) {
        return NDS_ERR_ARM9_BOUNDS;
    }
    if (range_overlaps(h.arm9_ram_addr, h.arm9_size,
                       bounds->reserve_base, bounds->reserve_end)) {
        return NDS_ERR_OVERLAP_BOOTLOADER;
    }

    /* --- ARM7 target may be Main RAM or ARM7-private WRAM ------------ */
    if (range_within(h.arm7_ram_addr, h.arm7_size,
                     bounds->ram_base, bounds->ram_end)) {
        if (range_overlaps(h.arm7_ram_addr, h.arm7_size,
                           bounds->reserve_base, bounds->reserve_end)) {
            return NDS_ERR_OVERLAP_BOOTLOADER;
        }
    } else if (range_within(h.arm7_ram_addr, h.arm7_size,
                            bounds->arm7_wram_base, bounds->arm7_wram_end)) {
        /* WRAM target: staged via IPC into ARM7-private memory. */
    } else {
        return NDS_ERR_ARM7_BOUNDS;
    }
    if (!addr_in_range(h.arm7_entry, h.arm7_ram_addr, h.arm7_size)) {
        return NDS_ERR_ARM7_BOUNDS;
    }

    return NDS_LOAD_OK;
}

/*
 * Validate a single optional TWL segment. `err` is the status to report when
 * the segment is malformed or lands outside the accepted RAM windows.
 */
static nds_load_status_t validate_twl_segment(u32 rom_off, u32 ram, u32 size,
                                              u32 file_size,
                                              const nds_load_bounds_t *bounds,
                                              nds_load_status_t err)
{
    bool in_ram;
    bool in_nwram;

    /* A missing segment (size 0) is always fine. */
    if (size == 0u) {
        return NDS_LOAD_OK;
    }

    if (!aligned_nonzero(rom_off) || !aligned_nonzero(ram) ||
        !aligned_nonzero(size)) {
        return err;
    }

    if (file_size != 0u && !file_range_ok(rom_off, size, file_size)) {
        return NDS_ERR_FILE_TRUNCATED;
    }

    in_ram   = range_within(ram, size, bounds->ram_base, bounds->ram_end);
    in_nwram = range_within(ram, size, bounds->nwram_base, bounds->nwram_end);
    if (!in_ram && !in_nwram) {
        return err;
    }

    if (range_overlaps(ram, size, bounds->reserve_base, bounds->reserve_end)) {
        return NDS_ERR_OVERLAP_BOOTLOADER;
    }

    return NDS_LOAD_OK;
}

nds_load_status_t nds_header_validate_twl_segments(const nds_header_t *hdr,
                                                   u32 file_size,
                                                   const nds_load_bounds_t *bounds)
{
    nds_load_bounds_t def;
    nds_load_status_t st;

    if (hdr == NULL) {
        return NDS_ERR_FILE_TRUNCATED;
    }
    if (bounds == NULL) {
        nds_load_bounds_default(&def, true);
        bounds = &def;
    }

    /* A non-TWL image simply has no extended segments. */
    if (!hdr->twl_capable) {
        return NDS_LOAD_OK;
    }

    st = validate_twl_segment(hdr->arm9i_rom_offset, hdr->arm9i_ram_address,
                              hdr->arm9i_size, file_size, bounds,
                              NDS_ERR_ARM9_BOUNDS);
    if (st != NDS_LOAD_OK) {
        return st;
    }

    return validate_twl_segment(hdr->arm7i_rom_offset, hdr->arm7i_ram_address,
                                hdr->arm7i_size, file_size, bounds,
                                NDS_ERR_ARM7_BOUNDS);
}

const char *nds_load_status_text(nds_load_status_t status)
{
    switch (status) {
    case NDS_LOAD_OK:                return "LOADABLE";
    case NDS_ERR_BAD_CRC:            return "BAD HEADER CRC16";
    case NDS_ERR_ARM9_BOUNDS:        return "ARM9 RANGE INVALID";
    case NDS_ERR_ARM7_BOUNDS:        return "ARM7 RANGE INVALID";
    case NDS_ERR_OVERLAP_BOOTLOADER: return "RANGE HITS BOOTLOADER";
    case NDS_ERR_FILE_TRUNCATED:     return "FILE TRUNCATED";
    default:                         return "UNKNOWN";
    }
}
