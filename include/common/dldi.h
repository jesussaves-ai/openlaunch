/*
 * include/common/dldi.h
 *
 * Clean-room DLDI (Dynamically Linked Device Interface) section scanner and
 * in-RAM patcher, plus the built-in read-only stub driver.
 *
 * DLDI is Chishm's position-independent storage-driver format used by libfat.
 * A homebrew .nds reserves a block of RAM for a "DLDI section" and marks it
 * with a fixed 12-byte signature; a loader replaces that block with a real
 * device driver so the running application can reach its medium. This module
 * only understands the on-disk/on-RAM structure — it never touches hardware.
 *
 * Canonical header (GBATEK "DS Cart DLDI Driver"):
 *
 *   0x00  u32   magic            0xBF8DA5ED
 *   0x04  u8[8] " Chishm\0"      signature string (space + 6 chars + NUL)
 *   0x0C  u8    version          format version (1)
 *   0x0D  u8    driver_size_log2 log2(bytes of .dldi + BSS)
 *   0x0E  u8    fix_flags        DLDI_FIX_* bitmask
 *   0x0F  u8    allocated_size_log2 log2(RAM reserved in the .nds)
 *   0x10  char[48] friendly_name NUL-padded driver name
 *   0x40  u32   data_start      ]  address table (link-time addresses);
 *   0x44  u32   data_end        ]  data/glue/GOT are relocated by the delta
 *   0x48  u32   glue_start      ]  between the destination and the driver,
 *   0x4C  u32   glue_end        ]  BSS is zero-filled
 *   0x50  u32   got_start       ]
 *   0x54  u32   got_end         ]
 *   0x58  u32   bss_start       ]
 *   0x5C  u32   bss_end         ]
 *   0x60  char[4] io_type       short interface tag (e.g. "NONE", "DSiR")
 *   0x64  u32   features        FEATURE_* bitmask
 *   0x68  u32   startup         ]  6 IO_INTERFACE function pointers
 *   0x6C  u32   is_inserted     ]
 *   0x70  u32   read_sectors    ]
 *   0x74  u32   write_sectors   ]
 *   0x78  u32   clear_status    ]
 *   0x7C  u32   shutdown        ]
 *   0x80  ...   driver code
 *
 * Safety (the project conventions§5): every routine is pure and fully bounds-checked; no
 * hardware register, DMA engine or eMMC/NAND path is involved. The built-in
 * stub is read-only and returns a safe failure for every I/O request.
 */
#ifndef DSI_COMMON_DLDI_H
#define DSI_COMMON_DLDI_H

#include "types.h"

/* The 12-byte signature: u32 magic followed by the 8-byte " Chishm\0". */
#define DLDI_MAGIC       0xBF8DA5EDu
#define DLDI_MAGIC_LEN   12u
#define DLDI_STRING_LEN  8u

/* The canonical DLDI header occupies 128 bytes; the driver code follows it. */
#define DLDI_HEADER_SIZE 128u

/* Header field offsets (see the layout above). */
#define DLDI_OFF_MAGIC        0x00u
#define DLDI_OFF_STRING       0x04u
#define DLDI_OFF_VERSION      0x0Cu
#define DLDI_OFF_DRIVER_SIZE  0x0Du
#define DLDI_OFF_FIX_FLAGS    0x0Eu
#define DLDI_OFF_ALLOC_SIZE   0x0Fu
#define DLDI_OFF_NAME         0x10u
#define DLDI_OFF_DATA_START   0x40u
#define DLDI_OFF_DATA_END     0x44u
#define DLDI_OFF_GLUE_START   0x48u
#define DLDI_OFF_GLUE_END     0x4Cu
#define DLDI_OFF_GOT_START    0x50u
#define DLDI_OFF_GOT_END      0x54u
#define DLDI_OFF_BSS_START    0x58u
#define DLDI_OFF_BSS_END      0x5Cu
#define DLDI_OFF_IO_TYPE      0x60u
#define DLDI_OFF_FEATURES     0x64u
#define DLDI_OFF_STARTUP      0x68u
#define DLDI_OFF_IS_INSERTED  0x6Cu
#define DLDI_OFF_READ         0x70u
#define DLDI_OFF_WRITE        0x74u
#define DLDI_OFF_CLEAR        0x78u
#define DLDI_OFF_SHUTDOWN     0x7Cu

#define DLDI_VERSION_VALUE    1u

/* fix_flags bits. */
#define DLDI_FIX_ALL   0x01u   /* relocate any in-range pointer in data      */
#define DLDI_FIX_GLUE  0x02u   /* relocate in-range pointers in the glue     */
#define DLDI_FIX_GOT   0x04u   /* relocate in-range pointers in the GOT      */
#define DLDI_FIX_BSS   0x08u   /* zero-fill the BSS range                    */

/* Sane bounds for the RAM a .nds reserves: 1 KiB .. 64 KiB. */
#define DLDI_ALLOC_LOG2_MIN  10u
#define DLDI_ALLOC_LOG2_MAX  16u

#define DLDI_NAME_LEN        48u
#define DLDI_IO_TYPE_LEN     4u

/*
 * Decoded DLDI header. Strings are always NUL-terminated and sanitised so they
 * are safe to render with the console.
 */
typedef struct {
    u8   version;
    u8   driver_size_log2;      /* log2(.dldi + BSS)                        */
    u8   fix_flags;             /* DLDI_FIX_* bitmask                       */
    u8   allocated_size_log2;   /* log2(RAM reserved in the target)         */
    u32  driver_size;           /* 1u << driver_size_log2                   */
    u32  allocated_size;        /* 1u << allocated_size_log2                */

    char friendly_name[DLDI_NAME_LEN + 1u];
    char io_type[DLDI_IO_TYPE_LEN + 1u];

    u32  data_start, data_end;
    u32  glue_start, glue_end;
    u32  got_start,  got_end;
    u32  bss_start,  bss_end;
    u32  fn_startup, fn_is_inserted, fn_read_sectors,
         fn_write_sectors, fn_clear_status, fn_shutdown;
} dldi_info_t;

/*
 * Scan `arm9_buf` (`arm9_len` bytes) on 4-byte boundaries for the 12-byte DLDI
 * signature, requiring room for a whole header after the match. Returns true
 * and stores the byte offset in `*out_offset` (when non-NULL) on success.
 * Pure and bounds-checked.
 */
bool dldi_find_offset(const u8 *arm9_buf, u32 arm9_len, u32 *out_offset);

/*
 * Decode a DLDI header from `buf` (`len` readable bytes). Validates the magic
 * and that `allocated_size_log2` is within [DLDI_ALLOC_LOG2_MIN,
 * DLDI_ALLOC_LOG2_MAX] and that the reserved block fits inside `len` (for a
 * target stub, `len` is the remaining ARM9 image after the stub offset). Fills
 * `*out` (when non-NULL). Pure and bounds-checked.
 */
bool dldi_parse_header(const u8 *buf, u32 len, dldi_info_t *out);

/*
 * Find and parse the DLDI stub in `buf`. Convenience wrapper around
 * dldi_find_offset() + dldi_parse_header(). Fills `*out` (optional) and
 * `*out_offset` (optional).
 */
bool dldi_scan(const u8 *buf, u32 len, dldi_info_t *out, u32 *out_offset);

/*
 * Patch `arm9_buf` (`arm9_len` bytes, staged in RAM at `arm9_ram_base`) by
 * copying `driver_buf` (`driver_len` bytes) over the target's DLDI stub and
 * relocating its address table.
 *
 * Both the target stub and the replacement driver are validated. The driver's
 * `driver_size_log2` must be <= the target's `allocated_size_log2`, and
 * `driver_len` must fit both the driver's own size and the target's reserved
 * block. Address fields are shifted by
 * `(arm9_ram_base + stub_offset) - driver.data_start`; the data/glue/GOT
 * ranges are walked when the matching FIX_* flag is set, and the BSS range is
 * zero-filled under DLDI_FIX_BSS.
 *
 * Returns true on success. Pure with respect to hardware: it only rewrites the
 * caller's buffer.
 */
bool dldi_apply_patch(u8 *arm9_buf, u32 arm9_len, u32 arm9_ram_base,
                      const u8 *driver_buf, u32 driver_len);

/* ---- Built-in read-only stub driver ----------------------------------- */

/*
 * Size of the built-in driver image (1 KiB) and the arbitrary link base its
 * header records. The base cancels out during relocation, so any 4-byte
 * aligned value works; the canonical 0xBF800000 is used for familiarity.
 */
#define DLDI_BUILTIN_RO_SIZE   0x400u
#define DLDI_BUILTIN_RO_BASE   0xBF800000u
#define DLDI_BUILTIN_RO_IO_TYPE "DSiR"

/*
 * Return the built-in, read-only clean-room DLDI stub driver template (a
 * DLDI_BUILTIN_RO_SIZE-byte image). It reports IOType "DSiR" and its function
 * pointers reference tiny ARM stubs inside the same image: startup,
 * is_inserted, clear_status and shutdown report success, while read_sectors
 * and write_sectors fail safely (no medium is ever written). The bytes are
 * built once into a static buffer and are safe to copy with dldi_apply_patch().
 */
const u8 *dldi_builtin_ro_driver(void);

#endif /* DSI_COMMON_DLDI_H */
