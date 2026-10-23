/*
 * include/arm9/nand_boot.h
 *
 * ARM9 glue for the read-only DSi NAND: the delegated eMMC sector reader and
 * the probe / mount / launcher-resolution helpers used by the boot menu.
 *
 * The eMMC NAND controller (TMIO port 1) is reachable only from the ARM7, so
 * every transfer here goes through arm9_storage_read_sectors() exactly like the
 * SD reader. No write path exists (the project conventions): the NAND is read-only.
 */
#ifndef DSI_ARM9_NAND_BOOT_H
#define DSI_ARM9_NAND_BOOT_H

#include "../common/types.h"
#include "../common/fat_ro.h"
#include "../common/nand.h"

/* Result of the LBA-0 NAND partition-table probe. */
typedef struct {
    bool ok;             /* the delegated LBA-0 read completed           */
    bool signature_ok;   /* the 0xAA55 MBR signature was present         */
    u16  signature;      /* raw little-endian word at offset 0x1FE       */
    u8   format;         /* nand_table_format_t of the decoded table     */
    u32  partitions;     /* non-empty partition entries                  */
    bool fat_present;    /* a FAT16/FAT32 partition was located          */
    u32  fat_lba;        /* first sector of that partition (0 when none) */
    u32  fat_sectors;    /* its size in 512-byte sectors                 */
    u32  total_sectors;  /* end sector of the last non-empty entry       */
} arm9_nand_probe_t;

/* Read `count` 512-byte NAND sectors from `lba` into shared Main RAM `dst`. */
bool arm9_nand_read_sectors(u32 lba, void *dst, u32 count);

/*
 * Read NAND LBA 0 through the ARM7 and decode its partition table. Never
 * returns a pointer to the internal buffer. Read-only.
 */
void arm9_nand_probe(arm9_nand_probe_t *out);

/*
 * Mount the FAT16/FAT32 volume on the NAND at `partition_lba` (0 lets
 * fat_mount() auto-detect the MBR partition). `vol` MUST live in shared Main
 * RAM. Returns true on success.
 */
bool arm9_nand_mount_fat(fat_volume_t *vol, u32 partition_lba);

/*
 * Search the mounted NAND volume for the DSi System Menu / HiyaCFW launcher.
 * On success copies the resolved path into `path_out` and returns true.
 */
bool arm9_nand_find_launcher(fat_volume_t *vol, char *path_out, u32 cap);

#endif /* DSI_ARM9_NAND_BOOT_H */
