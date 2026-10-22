/*
 * include/common/fat_ro.h
 *
 * Zero-allocation, bounds-checked, read-only FAT16 / FAT32 parser.
 *
 * The parser is deliberately decoupled from the storage backend: the caller
 * supplies a sector-read callback (on the ARM9 this is the ARM7-gated IPC
 * sector read), and all state lives in caller-owned structs — no malloc, no
 * global tables, no unbounded recursion.
 *
 * Design rules:
 *   - every on-disk field is validated before it is used;
 *   - every sector read is checked; a failure sets `fat_volume.io_error` and
 *     aborts the operation (fail-closed);
 *   - all cluster indices are bounds-checked against the volume geometry.
 *
 * FAT12 is intentionally not supported (a FAT12 volume is rejected at mount).
 */
#ifndef DSI_COMMON_FAT_RO_H
#define DSI_COMMON_FAT_RO_H

#include "types.h"

#define FAT_SECTOR_SIZE   512u
#define FAT_DIR_ENTRY_SIZE 32u
#define FAT_DIR_ENTRIES_PER_SECTOR (FAT_SECTOR_SIZE / FAT_DIR_ENTRY_SIZE)
#define FAT_NAME_MAX      13u   /* "NAME.EXT" + NUL                        */

/* Partition types that carry a FAT16/FAT32 volume. */
#define FAT_PART_TYPE_FAT16_SMALL 0x04u
#define FAT_PART_TYPE_FAT16       0x06u
#define FAT_PART_TYPE_FAT32       0x0Bu
#define FAT_PART_TYPE_FAT32_LBA   0x0Cu
#define FAT_PART_TYPE_FAT16_LBA   0x0Eu

/*
 * Sector-read callback. `lba` is an absolute sector number on the medium.
 * Must write exactly FAT_SECTOR_SIZE bytes into `out` and return false on any
 * error (including an out-of-range LBA).
 */
typedef bool (*fat_read_sector_fn)(void *ctx, u32 lba,
                                   u8 out[FAT_SECTOR_SIZE]);

typedef enum {
    FAT_TYPE_NONE = 0,
    FAT_TYPE_FAT16,
    FAT_TYPE_FAT32
} fat_type_t;

/*
 * Mounted-volume geometry and scratch state. This struct is large (it carries
 * one sector buffer), so it should be a static / long-lived object. On the
 * ARM9 it MUST live in shared Main RAM: the sector buffer is filled directly by
 * the ARM7 over IPC.
 */
typedef struct fat_volume {
    fat_read_sector_fn read;
    void *ctx;

    fat_type_t type;
    u16 bytes_per_sector;      /* always 512 for a supported volume        */
    u8  sectors_per_cluster;   /* power of two, 1..128                     */
    u16 reserved_sectors;
    u8  num_fats;              /* >= 1                                     */
    u16 root_entry_count;      /* FAT16 only; 0 for FAT32                  */
    u32 total_sectors;
    u32 fat_size;              /* sectors per FAT                          */
    u32 root_cluster;          /* FAT32 only                               */

    u32 partition_lba;         /* LBA of the BPB (0 = superfloppy)         */
    u32 fat_start_lba;         /* first FAT table                          */
    u32 root_start_lba;        /* FAT16 fixed root directory               */
    u32 root_sectors;          /* FAT16 fixed root directory size          */
    u32 data_start_lba;        /* first data sector (cluster 2)            */
    u32 cluster_count;         /* number of data clusters                  */
    u32 max_cluster;           /* highest valid cluster (cluster_count+1)  */

    bool io_error;             /* set when a sector read fails             */
    u8  sector_buf[FAT_SECTOR_SIZE];
} fat_volume_t;

/* One parsed directory entry (short 8.3 name only). */
typedef struct fat_file_entry {
    char name[FAT_NAME_MAX];   /* upper-case "NAME.EXT", NUL-terminated    */
    u8   attributes;
    u32  start_cluster;
    u32  file_size;
} fat_file_entry_t;

/*
 * Directory iterator. `fat_dir_iter_init()` starts at the root directory
 * (FAT16 fixed area or the FAT32 root cluster chain); `fat_dir_next()` walks
 * it one regular file at a time.
 */
typedef struct fat_dir_iter {
    fat_volume_t *vol;
    bool nds_only;             /* skip files that are not .NDS / .SRL      */
    bool is_root_fixed;        /* FAT16 fixed root area                    */
    bool buffer_valid;         /* vol->sector_buf holds the current sector */
    bool finished;
    u32  root_sectors_left;    /* FAT16 fixed root: sectors not yet read   */
    u32  cur_cluster;          /* current cluster (FAT32 / cluster chains) */
    u32  sectors_left_in_cluster;
    u32  cur_lba;              /* next sector to read                      */
    u32  entry_index;          /* next entry within the buffered sector    */
} fat_dir_iter_t;

/* ---- Mount / geometry -------------------------------------------------- */

/*
 * Probe the medium and mount the first FAT16/FAT32 volume.
 *
 * Reads LBA 0; if it is an MBR with a recognised FAT partition entry, the BPB
 * is taken from that partition's start LBA, otherwise LBA 0 is assumed to hold
 * the BPB directly (superfloppy). Returns false if no usable volume is found.
 */
bool fat_mount(fat_volume_t *vol, fat_read_sector_fn read_fn, void *ctx);

/* ---- FAT chain --------------------------------------------------------- */

/*
 * Return the next cluster in the chain, or 0xFFFFFFFF on error / invalid
 * input. Use fat_is_eoc() to detect an end-of-chain marker.
 */
u32 fat_get_next_cluster(fat_volume_t *vol, u32 cluster);

/* True when `cluster` is an end-of-chain marker for the mounted type. */
bool fat_is_eoc(const fat_volume_t *vol, u32 cluster);

/* ---- Directory listing ------------------------------------------------- */

/* Begin iterating the root directory. `nds_only` restricts files to .NDS/.SRL. */
void fat_dir_iter_init(fat_volume_t *vol, fat_dir_iter_t *it, bool nds_only);

/*
 * Fetch the next regular file. Long-file-name entries, volume labels, deleted
 * entries and subdirectories are skipped. Returns false at the end of the
 * directory or on error.
 */
bool fat_dir_next(fat_dir_iter_t *it, fat_file_entry_t *out);

/* True when the entry name ends in .NDS or .SRL (case-insensitive). */
bool fat_is_nds_entry(const fat_file_entry_t *entry);

/* ---- File data --------------------------------------------------------- */

/*
 * Read up to `max_len` bytes of `entry` into `dest_buf`. Traverses the cluster
 * chain, respecting `file_size`. On return `*out_len` (if non-NULL) holds the
 * number of bytes written. Returns true only when the requested amount was
 * read in full.
 */
bool fat_read_file(fat_volume_t *vol, const fat_file_entry_t *entry,
                   u8 *dest_buf, u32 max_len, u32 *out_len);

#endif /* DSI_COMMON_FAT_RO_H */
