/*
 * include/arm9/storage.h
 *
 * ARM9 client for the ARM7-gated storage and firmware devices.
 *
 * Architectural rule (the project conventions): the TMIO SD/eMMC controller and the firmware
 * SPI flash are reachable only from the ARM7. The ARM9 must never touch their
 * registers; it packages an operation into an IPC request and blocks for the
 * reply. These wrappers are the only sanctioned entry points on the ARM9 side.
 */
#ifndef DSI_ARM9_STORAGE_H
#define DSI_ARM9_STORAGE_H

#include "../common/types.h"
#include "../common/ipc_protocol.h"
#include "../common/fat_ro.h"

/* Per-request poll budget (~tens of ms at 134 MHz). */
#ifndef ARM9_STORAGE_IPC_TIMEOUT
#define ARM9_STORAGE_IPC_TIMEOUT 0x00200000u
#endif

/*
 * Read `count` 512-byte sectors starting at `lba` from `device`
 * (IPC_STORAGE_DEV_SD / IPC_STORAGE_DEV_EMMC) into `dst`.
 *
 * `dst` MUST point into shared Main RAM (IPC_SHARED_RAM_BASE..IPC_SHARED_RAM_END)
 * and MUST NOT be in DTCM: the ARM7 writes to it directly and DTCM is not
 * visible to the ARM7. The ARM9 data cache is flushed before the request and
 * invalidated afterwards. Returns true when the ARM7 reports IPC_IO_FLAG_OK.
 */
bool arm9_storage_read_sectors(u32 device, u32 lba, void *dst, u32 count);

#ifdef ENABLE_NAND_WRITE_DANGER
/*
 * Write `count` 512-byte sectors from `src` to `device`
 * (only meaningful with ENABLE_NAND_WRITE_DANGER defined; the project conventions).
 * `src` MUST be in shared Main RAM. The SD card is written for the mandatory
 * backup; the eMMC is written only after the ARM7 write guard is armed.
 * Returns true when the ARM7 reports IPC_IO_FLAG_OK.
 */
bool arm9_storage_write_sectors(u32 device, u32 lba, const void *src, u32 count);
#endif

/*
 * Read `len` bytes (<= IPC_FW_MAX_BYTES) from the firmware SPI flash at the
 * 24-bit `address` into `dst` (shared Main RAM). Returns true on success.
 */
bool arm9_firmware_read(u32 address, void *dst, u32 len);

/* Result of the LBA-0 round-trip smoke test. */
typedef struct {
    bool ok;             /* the delegated transfer completed            */
    bool mbr_signature;  /* bytes 510/511 read back as 55 AA             */
    u16  signature;      /* raw little-endian word at offset 510         */
    u8   fs_type;        /* fat_type_t of the volume (FAT_TYPE_*)        */
    u8   partition_type; /* MBR partition type byte (0 if superfloppy)   */
    u32  partition_lba;  /* first sector of the FAT volume               */
    u32  total_sectors;  /* volume size in 512-byte sectors              */
} arm9_storage_probe_t;

/*
 * Read sector 0 (the MBR) from the SD card into a static shared-RAM buffer and
 * check for the 0x55AA boot signature. This exercises the whole delegated
 * pipeline: ARM9 -> IPC request -> ARM7 -> TMIO -> FIFO -> shared Main RAM ->
 * ARM9 cache invalidate. Never returns a pointer to the buffer.
 */
void arm9_storage_probe(arm9_storage_probe_t *out);

/*
 * Mount a read-only FAT16/FAT32 volume from `device` using the delegated
 * sector reader as the FAT backend. `vol` MUST live in shared Main RAM (its
 * sector buffer is filled directly by the ARM7). Returns true on success.
 */
bool arm9_storage_mount(fat_volume_t *vol, u32 device);

#endif /* DSI_ARM9_STORAGE_H */
