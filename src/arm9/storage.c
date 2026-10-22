/*
 * src/arm9/storage.c
 *
 * ARM9 clients for the ARM7-gated TMIO SD/eMMC controller and the firmware SPI
 * flash. See include/arm9/storage.h and include/common/ipc_protocol.h.
 *
 * The ARM9 never issues a hardware access here: each call packs the operation
 * into a parameterised IPC request, waits for the ARM7 acknowledgement, and
 * maintains the ARM9 data cache around the shared buffer. This is the only
 * ARM9 code allowed to request storage/SPI work (AGENTS.md architectural rule).
 */
#include "arm9/storage.h"
#include "arm9/ipc.h"
#include "common/cache.h"

/*
 * Send one delegated read request and return its result-flag word
 * (IPC_IO_FLAG_*). Any transport failure is reported as IPC_IO_FLAG_ERROR.
 */
static u32 storage_request(u32 cmd, const u32 *args, u32 nargs)
{
    u32 reply[IPC_REPLY_READ_WORDS + 1u];
    u32 got;

    got = arm9_ipc_query_args(IPC_CH_SYSTEM, cmd, args, nargs,
                              reply, ARRAY_SIZE(reply),
                              ARM9_STORAGE_IPC_TIMEOUT);
    if (got < ARRAY_SIZE(reply)) {
        return IPC_IO_FLAG_ERROR;
    }
    return reply[1];
}

bool arm9_storage_read_sectors(u32 device, u32 lba, void *dst, u32 count)
{
    u32 args[IPC_REQ_READ_SECTOR_WORDS];
    u32 len;
    u32 flags;

    if (dst == NULL || count == 0u || count > IPC_STORAGE_MAX_SECTORS) {
        return false;
    }

    len = count * IPC_STORAGE_SECTOR_SIZE;
    if (!ipc_addr_is_shared_ram((u32)(uintptr_t)dst, len)) {
        return false;
    }

    /* Clean (and drop) stale lines before the ARM7 fills the buffer. */
    dc_flush_range(dst, len);

    args[0] = device;
    args[1] = lba;
    args[2] = (u32)(uintptr_t)dst;
    args[3] = count;

    flags = storage_request(IPC_CMD_READ_SECTOR, args,
                            IPC_REQ_READ_SECTOR_WORDS);

    /* The ARM7 wrote the buffer; discard cached lines so it is re-read. */
    dc_invalidate_range(dst, len);

    return (flags & IPC_IO_FLAG_OK) != 0u;
}

bool arm9_firmware_read(u32 address, void *dst, u32 len)
{
    u32 args[IPC_REQ_READ_FIRMWARE_WORDS];
    u32 flags;

    if (dst == NULL || len == 0u || len > IPC_FW_MAX_BYTES) {
        return false;
    }
    if (!ipc_addr_is_shared_ram((u32)(uintptr_t)dst, len)) {
        return false;
    }

    dc_flush_range(dst, len);

    args[0] = address;
    args[1] = (u32)(uintptr_t)dst;
    args[2] = len;

    flags = storage_request(IPC_CMD_READ_FIRMWARE, args,
                            IPC_REQ_READ_FIRMWARE_WORDS);

    dc_invalidate_range(dst, len);

    return (flags & IPC_IO_FLAG_OK) != 0u;
}

/*
 * FAT backend: read one 512-byte sector through the ARM7-gated IPC path. The
 * device selector travels in `ctx` (the callback never dereferences it).
 */
static bool sd_read_sector(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    u32 device = (u32)(uintptr_t)ctx;

    return arm9_storage_read_sectors(device, lba, out, 1u);
}

bool arm9_storage_mount(fat_volume_t *vol, u32 device)
{
    if (vol == NULL) {
        return false;
    }
    return fat_mount(vol, sd_read_sector, (void *)(uintptr_t)device);
}

/*
 * LBA-0 smoke-test buffer and probe volume. Both live in the ARM9 image's
 * .bss, i.e. shared Main RAM (0x0220xxxx), which the ARM7 can write directly.
 * Must NOT be in DTCM.
 */
static u8 s_sector0[IPC_STORAGE_SECTOR_SIZE];
static fat_volume_t s_probe_vol;

/*
 * Read LBA 0 (the MBR / VBR), check the 0xAA55 boot signature, locate the FAT
 * partition (if the sector is an MBR) and classify the volume as FAT16/FAT32
 * with its total size. Everything is read-only and delegated to the ARM7.
 */
void arm9_storage_probe(arm9_storage_probe_t *out)
{
    if (out == NULL) {
        return;
    }

    out->ok = false;
    out->mbr_signature = false;
    out->signature = 0u;
    out->fs_type = FAT_TYPE_NONE;
    out->partition_type = 0u;
    out->partition_lba = 0u;
    out->total_sectors = 0u;

    if (!arm9_storage_read_sectors(IPC_STORAGE_DEV_SD, 0u, s_sector0, 1u)) {
        return;
    }

    out->ok = true;
    out->signature = (u16)((u16)s_sector0[510] |
                           ((u16)s_sector0[511] << 8));
    out->mbr_signature = (out->signature == 0xAA55u);

    {
        fat_mbr_partition_t part;

        if (fat_mbr_find_fat_partition(s_sector0, &part)) {
            out->partition_type = part.type;
            out->partition_lba = part.lba;
        }
    }

    /* Classify the volume (FAT16/FAT32) and read its size. */
    if (fat_mount(&s_probe_vol, sd_read_sector,
                  (void *)(uintptr_t)IPC_STORAGE_DEV_SD)) {
        out->fs_type = (u8)s_probe_vol.type;
        out->total_sectors = s_probe_vol.total_sectors;
        if (out->partition_lba == 0u) {
            out->partition_lba = s_probe_vol.partition_lba;
        }
    }
}

#ifdef ENABLE_NAND_WRITE_DANGER
/*
 * Delegated sector write. The SD card is written for the mandatory backup;
 * the eMMC is written only when the ARM7 write guard is armed (the ARM7
 * refuses IPC_STORAGE_DEV_EMMC otherwise). The source buffer is cleaned to
 * main memory before the request so the ARM7 (which has no data cache) sees
 * the bytes. Never in DTCM.
 */
bool arm9_storage_write_sectors(u32 device, u32 lba, const void *src, u32 count)
{
    u32 args[IPC_REQ_WRITE_SECTOR_WORDS];
    u32 len;
    u32 flags;

    if (src == NULL || count == 0u || count > IPC_STORAGE_MAX_SECTORS) {
        return false;
    }

    len = count * IPC_STORAGE_SECTOR_SIZE;
    if (!ipc_addr_is_shared_ram((u32)(uintptr_t)src, len)) {
        return false;
    }

    dc_flush_range(src, len);

    args[0] = device;
    args[1] = lba;
    args[2] = (u32)(uintptr_t)src;
    args[3] = count;

    flags = storage_request(IPC_CMD_WRITE_SECTOR, args,
                            IPC_REQ_WRITE_SECTOR_WORDS);

    return (flags & IPC_IO_FLAG_OK) != 0u;
}
#endif /* ENABLE_NAND_WRITE_DANGER */
