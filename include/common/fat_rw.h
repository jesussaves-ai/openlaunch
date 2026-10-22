/*
 * include/common/fat_rw.h
 *
 * Minimal, bounded FAT16/FAT32 *writer*: create one file in the volume root
 * and stream its contents. This exists solely so the installer can drop its
 * mandatory NAND backup onto the SD card as a plain file (nand_backup.bin).
 *
 * It is deliberately narrow:
 *   - only the root directory is written (no subdirectories, no LFN);
 *   - only 8.3 names, upper-cased;
 *   - one file at a time, sequential append, no overwrite (a name clash fails
 *     closed rather than clobbering an existing backup);
 *   - every geometry value comes from an already-mounted fat_volume_t, and
 *     every write is checked; a failure aborts and frees what it allocated.
 *
 * Writing to the *SD card* is not a NAND operation, so this module is not
 * gated by ENABLE_NAND_WRITE_DANGER; the caller (src/arm9/installer_backup.c)
 * is. The pure logic stays in common/ so it links into the host test suite.
 */
#ifndef DSI_COMMON_FAT_RW_H
#define DSI_COMMON_FAT_RW_H

#include "types.h"
#include "fat_ro.h"

/*
 * 512-byte sector-write callback. Must persist exactly FAT_SECTOR_SIZE bytes
 * to absolute `lba` and return false on any error.
 */
typedef bool (*fat_write_sector_fn)(void *ctx, u32 lba,
                                    const u8 sector[FAT_SECTOR_SIZE]);

typedef enum {
    FAT_WR_OK = 0,
    FAT_WR_ERR_ARG,        /* invalid argument / unmounted volume          */
    FAT_WR_ERR_NAME,       /* name is not a valid 8.3 name                  */
    FAT_WR_ERR_IO,         /* a sector read/write failed                    */
    FAT_WR_ERR_EXISTS,     /* a file with that name already exists          */
    FAT_WR_ERR_NO_SPACE,   /* no free directory slot or data cluster        */
    FAT_WR_ERR_BAD_CLUSTER /* corrupt FAT chain encountered while freeing   */
} fat_write_status_t;

/*
 * Sequential file writer. Large (carries a 512-byte scratch sector), so keep
 * it static / long-lived; on the ARM9 it must live in shared Main RAM.
 */
typedef struct {
    fat_volume_t *vol;
    fat_write_sector_fn write;
    void *ctx;

    bool active;
    u32  dir_lba;          /* sector holding the directory entry           */
    u32  dir_off;          /* byte offset of the entry inside that sector  */
    u32  first_cluster;    /* 0 for an empty file                          */
    u32  cur_cluster;      /* cluster currently being filled               */
    u32  cluster_offset;   /* bytes already used in `cur_cluster`          */
    u32  total_bytes;      /* bytes written so far                         */
    u32  alloc_hint;       /* first cluster to probe for a free slot       */

    u8   buf[FAT_SECTOR_SIZE];
} fat_writer_t;

/*
 * Create a new root file. `name` may be "NANDBACK.BIN" or "nandback.bin" (the
 * writer upper-cases it). Fails with FAT_WR_ERR_EXISTS when the name is taken.
 */
fat_write_status_t fat_create_file(fat_volume_t *vol,
                                   fat_write_sector_fn write, void *ctx,
                                   const char *name, fat_writer_t *w);

/* Append `len` bytes. Allocates and links clusters as the file grows. */
fat_write_status_t fat_writer_write(fat_writer_t *w, const u8 *data, u32 len);

/*
 * Finalise: rewrite the directory entry with the true size and mark the writer
 * inactive. `out_size` (optional) receives the final byte count.
 */
fat_write_status_t fat_writer_end(fat_writer_t *w, u32 *out_size);

/*
 * Abort: free every cluster allocated for this file and mark its directory
 * entry deleted. Safe to call at any point (including after end()).
 */
void fat_writer_abort(fat_writer_t *w);

/* Human-readable text for a write status (never NULL). */
const char *fat_write_status_text(fat_write_status_t status);

#endif /* DSI_COMMON_FAT_RW_H */
