/*
 * include/common/nand.h
 *
 * Read-only DSi NAND partition-table parser and NAND boot-redirect policy.
 *
 * The DSi NAND is a 240 MB eMMC whose first sector carries a PC-style MBR
 * (GBATEK "DSi SD/MMC Internal NAND Layout"). HiyaCFW redirects the NAND to an
 * SD/NAND image whose first sector uses a TWL-specific {offset, size} entry
 * form. Both are handled here so the bootloader can locate a FAT16/FAT32 NAND
 * volume (the DSi user partition / a HiyaCFW root) and, when the SD card offers
 * no boot target, fall back to the on-NAND system launcher.
 *
 * Everything in this module is pure and zero-allocation: a 512-byte sector in,
 * a decoded table out. The actual sector transfer is performed by the ARM9 glue
 * in src/arm9/nand_boot.c, which delegates every read to the ARM7 over IPC
 * (IPC_STORAGE_DEV_EMMC) exactly like the TMIO SD reader.
 */
#ifndef DSI_COMMON_NAND_H
#define DSI_COMMON_NAND_H

#include "types.h"
#include "fat_ro.h"

/* ---- PC-style MBR / TWL partition table geometry ---------------------- */
#define NAND_SECTOR_SIZE        512u
#define NAND_MBR_SIGNATURE      0xAA55u
#define NAND_MBR_SIG_OFF        0x1FEu
#define NAND_MBR_TABLE_OFF      0x1BEu
#define NAND_MBR_ENTRY_COUNT    4u
#define NAND_MBR_ENTRY_SIZE     16u

/* PC-style entry field offsets (within one 16-byte entry). */
#define NAND_MBR_TYPE_OFF       4u
#define NAND_MBR_LBA_OFF        8u
#define NAND_MBR_COUNT_OFF      12u

/* TWL/HiyaCFW entry field offsets: {offset, size} in 512-byte sectors. */
#define NAND_TWL_OFFSET_OFF     0u
#define NAND_TWL_SIZE_OFF       4u

/* ---- Launcher paths (DSi System Menu / HiyaCFW) ----------------------- */
/*
 * The DSi System Menu is the TWL title 00030004-484e4c41 ("HNLA"). HiyaCFW
 * keeps its redirected NAND root under "/rootfs" with titles below "/title".
 */
#define NAND_TITLE_SYSTEM_MENU_HI   0x00030004u
#define NAND_TITLE_SYSTEM_MENU_LO   0x484E4C41u   /* ASCII "HNLA"               */
#define NAND_SYSTEM_MENU_PATH       "/title/00030004/484e4c41/content/00000000.app"
#define NAND_HIYACFW_ROOTFS_PATH    "/rootfs"
#define NAND_HIYACFW_TITLE_PATH     "/title"
/* HiyaCFW keeps the redirected NAND tree under "/rootfs". */
#define NAND_HIYACFW_LAUNCHER_PATH \
    "/rootfs/title/00030004/484e4c41/content/00000000.app"

/* Longest launcher path buffer (incl. NUL). */
#define NAND_LAUNCHER_PATH_MAX      64u

/* Deep-path resolver bounds. */
#define NAND_PATH_MAX_DEPTH     8u
#define NAND_COMPONENT_MAX      13u   /* "NAME.EXT" + NUL */
#define NAND_DIR_PAGE           16u

/* Which table layout a sector carries. */
typedef enum {
    NAND_TABLE_NONE = 0,
    NAND_TABLE_PC_MBR,   /* standard 16-byte MBR entries (type/lba/count)   */
    NAND_TABLE_TWL       /* {offset, size} sector pairs (HiyaCFW / DSi)     */
} nand_table_format_t;

/* One decoded partition. */
typedef struct {
    bool present;        /* the entry was not all-zero                  */
    bool fat;            /* recognised FAT16/FAT32 partition            */
    u8   type;           /* raw MBR type byte (0 in the TWL form)       */
    u32  lba;            /* first 512-byte sector                       */
    u32  sector_count;   /* partition size in 512-byte sectors          */
} nand_partition_t;

/* A fully decoded partition table. */
typedef struct {
    nand_table_format_t format;
    u32 count;           /* non-empty entries                           */
    u32 fat_count;       /* FAT16/FAT32 entries                         */
    s32 largest_fat;     /* index of the largest FAT entry, or -1       */
    u32 total_sectors;   /* end sector of the highest non-empty entry   */
    nand_partition_t partitions[NAND_MBR_ENTRY_COUNT];
} nand_table_t;

/* True when `type` is a FAT16/FAT32 MBR partition type. */
bool nand_partition_is_fat(u8 type);

/*
 * Parse a PC-style MBR (signature 0x55AA, four 16-byte entries at 0x1BE).
 * Returns true when the signature is valid; `*out` is always populated (zeroed
 * on a signature mismatch). Pure.
 */
bool nand_parse_mbr_pc(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out);

/*
 * Parse the TWL {offset, size} entry form (four 16-byte entries at 0x1BE,
 * little-endian sector numbers). Returns true when the signature is valid.
 */
bool nand_parse_mbr_twl(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out);

/*
 * Auto-detect the table format: prefer a PC-style MBR with a FAT partition,
 * then a TWL {offset,size} table, then any PC-style entries. Returns false when
 * the signature is missing or no entry is present.
 */
bool nand_parse_table(const u8 sector[NAND_SECTOR_SIZE], nand_table_t *out);

/* First FAT partition index, or -1 when none. */
s32 nand_find_fat_partition(const nand_table_t *tbl);

/* Index of the largest FAT partition, or -1 when none. */
s32 nand_find_largest_fat(const nand_table_t *tbl);

/*
 * Resolve an absolute FAT path against `vol`, walking one component at a time.
 * Supports arbitrarily deep paths (bounded by NAND_PATH_MAX_DEPTH) using
 * fat_list_dir_cluster() for each directory level; matching is case-insensitive
 * on the 8.3 name. Returns true and fills `*out` when the final component
 * exists. Pure apart from the supplied sector-read backend.
 */
bool nand_resolve_path(fat_volume_t *vol, const char *path,
                       fat_file_entry_t *out);

/* ---- Geometry, pre-flight layout validation and write guards ---------- */

/*
 * Sanity bound on the DSi eMMC (256 MiB / 512 = 0x80000 sectors). The real
 * device is 240 MiB usable; this generous ceiling exists only so a corrupt or
 * malicious partition entry cannot describe a region past the chip.
 */
#define NAND_MAX_SECTORS        0x80000u
#define NAND_MIN_FAT_SECTORS    0x100u   /* 128 KiB: reject tiny/absurd FATs */

/*
 * Size of the protected boot prefix (in 512-byte sectors): the MBR sector and
 * the stage-1/stage-2 bootstrap area that physically precedes the user FAT
 * volume. The installer refuses to write any sector below the FAT partition
 * start, so this prefix is derived, not assumed.
 */
#define NAND_BOOT_GUARD_SECTORS 1u   /* the MBR/BPB sector itself, minimum */

/* Result of nand_validate_layout(). NAND_LAYOUT_OK == 0. */
typedef enum {
    NAND_LAYOUT_OK = 0,
    NAND_LAYOUT_ERR_NULL,       /* NULL argument                            */
    NAND_LAYOUT_ERR_SIGNATURE,  /* 0x55AA boot signature missing            */
    NAND_LAYOUT_ERR_EMPTY,      /* no non-empty partition entry             */
    NAND_LAYOUT_ERR_NO_FAT,     /* no FAT16/FAT32 partition located         */
    NAND_LAYOUT_ERR_TOO_SMALL,  /* FAT partition smaller than the minimum   */
    NAND_LAYOUT_ERR_BOUNDS      /* partition extends past NAND_MAX_SECTORS  */
} nand_layout_status_t;

/* A validated NAND layout: where the user FAT volume lives and where the
 * protected boot prefix ends. */
typedef struct {
    nand_table_format_t format;
    u32 partition_lba;      /* first sector of the user FAT volume          */
    u32 partition_sectors;  /* its size                                     */
    u32 total_sectors;      /* end sector of the highest non-empty entry    */
    u32 protected_end;      /* exclusive end of the write-protected prefix  */
    u32 total_backup_sectors; /* `total_sectors` (the full-image dump size)  */
} nand_layout_t;

/*
 * Pre-flight validation of a NAND LBA-0 sector.
 *
 * Verifies the 0x55AA signature, auto-detects the table format, requires a
 * FAT16/FAT32 partition (or, for the TWL form, a first partition confirmed by
 * the caller's mount), checks the geometry against `max_sectors` and fills
 * `*out`. Returns NAND_LAYOUT_OK on success. Pure (no device access) so the
 * installer can prove a layout is safe before it reads a single payload byte.
 */
nand_layout_status_t nand_validate_layout(const u8 sector[NAND_SECTOR_SIZE],
                                          u32 max_sectors,
                                          nand_layout_t *out);

/* Human-readable text for a layout status (never NULL). */
const char *nand_layout_status_text(nand_layout_status_t status);

/*
 * True when the half-open sector range [lba, lba + count) overlaps the
 * protected boot prefix of `layout`. Used as the final veto before any write.
 */
bool nand_region_is_protected(const nand_layout_t *layout, u32 lba, u32 count);

/* ---- Boot-redirect policy (pure) -------------------------------------- */

/* Inputs to the redirect decision. */
typedef struct {
    bool sd_card_present;   /* the SD slot reports a card                 */
    bool sd_target_found;   /* BOOT.NDS / a BOOT.CFG target resolved      */
    bool menu_forced;       /* B or SELECT held at startup                */
    bool nand_present;      /* a usable NAND FAT partition was located    */
} nand_boot_context_t;

/* The resolved redirect decision. */
typedef struct {
    bool attempt;           /* true when the loader should try NAND       */
    const char *path;       /* launcher path (static storage)             */
    const char *reason;     /* human-readable explanation (static)        */
} nand_boot_target_t;

/*
 * Decide whether to redirect boot to internal NAND.
 *
 * Policy (mirroring Unlaunch/HiyaCFW): a held B/SELECT always wins (menu); an
 * SD boot target always wins; otherwise, when no SD target exists and a NAND
 * FAT volume is present, redirect to the DSi System Menu launcher.
 */
void boot_resolve_nand_target(const nand_boot_context_t *ctx,
                              nand_boot_target_t *out);

#endif /* DSI_COMMON_NAND_H */
