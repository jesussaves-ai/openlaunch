/*
 * src/arm9/installer_backup.c
 *
 * Mandatory NAND backup + verification. See include/arm9/installer.h.
 *
 * The streaming/hashing core carries no hardware access and is exercised by
 * tests/test_installer.c. The `installer_backup_to_sd()` wrapper (compiled
 * only when ENABLE_NAND_WRITE_DANGER is defined) reads the eMMC through the
 * ARM7-gated IPC reader and writes a root file on the SD card through the
 * bounded FAT writer.
 */
#include "arm9/installer.h"
#include "common/fat_rw.h"
#include "common/mem.h"

/* Shared-RAM staging sector. Never DTCM: the ARM7 fills it over IPC. */
static u8 s_backup_sector[INSTALLER_SECTOR_SIZE];

/* Scratch for the SD read-back verification (shared Main RAM on the target). */
static u8 s_sd_verify_sector[INSTALLER_SECTOR_SIZE];

/* ---- Streaming core (host-testable) ----------------------------------- */

installer_error_t installer_backup_stream(installer_read_fn read, void *read_ctx,
                                          installer_sink_fn sink, void *sink_ctx,
                                          u32 start_lba, u32 sectors,
                                          installer_transfer_t *out)
{
    sha256_ctx_t sha;
    u32 i;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
        out->error = INSTALLER_ERR_BACKUP_FAILED;
    }
    if (read == NULL || sink == NULL || sectors == 0u) {
        return INSTALLER_ERR_BACKUP_FAILED;
    }

    sha256_init(&sha);
    for (i = 0u; i < sectors; i++) {
        if (!read(read_ctx, start_lba + i, s_backup_sector)) {
            return INSTALLER_ERR_BACKUP_FAILED;
        }
        sha256_update(&sha, s_backup_sector, INSTALLER_SECTOR_SIZE);
        if (!sink(sink_ctx, s_backup_sector, INSTALLER_SECTOR_SIZE)) {
            return INSTALLER_ERR_BACKUP_FAILED;
        }
        /* Observational only: the hook renders a progress bar but can never
         * change the transfer's outcome. */
        installer_progress_step(INSTALLER_STATE_BACKUP, i + 1u, sectors,
                                "backup");
    }
    sha256_final(&sha, out->digest);

    out->ok = true;
    out->lba = start_lba;
    out->sectors = sectors;
    out->bytes = sectors * INSTALLER_SECTOR_SIZE;
    out->error = INSTALLER_ERR_NONE;
    return INSTALLER_ERR_NONE;
}

installer_error_t installer_backup_verify(installer_read_fn read, void *read_ctx,
                                          const u8 expected[SHA256_DIGEST_SIZE],
                                          u32 start_lba, u32 sectors)
{
    sha256_ctx_t sha;
    u8 digest[SHA256_DIGEST_SIZE];
    u32 i;

    if (read == NULL || expected == NULL || sectors == 0u) {
        return INSTALLER_ERR_BACKUP_FAILED;
    }

    sha256_init(&sha);
    for (i = 0u; i < sectors; i++) {
        if (!read(read_ctx, start_lba + i, s_backup_sector)) {
            return INSTALLER_ERR_BACKUP_FAILED;
        }
        sha256_update(&sha, s_backup_sector, INSTALLER_SECTOR_SIZE);
        installer_progress_step(INSTALLER_STATE_VERIFY_BACKUP, i + 1u, sectors,
                                "verify");
    }
    sha256_final(&sha, digest);

    return sha256_digest_equal(digest, expected) ? INSTALLER_ERR_NONE
                                                 : INSTALLER_ERR_BACKUP_MISMATCH;
}

/* ---- SD file read-back verification (host-testable) ------------------- */

installer_error_t installer_backup_verify_sd(fat_volume_t *vol,
                                             const u8 expected[SHA256_DIGEST_SIZE],
                                             u32 expected_bytes,
                                             installer_transfer_t *out)
{
    fat_file_entry_t entry;
    sha256_ctx_t sha;
    u8 digest[SHA256_DIGEST_SIZE];
    u32 total_sectors;
    u32 i;

    if (out != NULL) {
        dsi_memset(out, 0, sizeof(*out));
        out->error = INSTALLER_ERR_BACKUP_MISMATCH;
    }
    /* Sector-aligned non-empty requests only (the backup is always aligned). */
    if (vol == NULL || expected == NULL || expected_bytes == 0u ||
        (expected_bytes % INSTALLER_SECTOR_SIZE) != 0u) {
        return INSTALLER_ERR_BACKUP_FAILED;
    }

    /*
     * The file must exist with the exact expected size. A dropped or truncated
     * SD write (e.g. a read-only emulated card) fails closed right here, before
     * any byte is hashed.
     */
    if (!fat_find_path(vol, INSTALLER_BACKUP_FILE, &entry) || entry.is_dir) {
        return INSTALLER_ERR_BACKUP_MISMATCH;
    }
    if (entry.file_size != expected_bytes) {
        return INSTALLER_ERR_BACKUP_MISMATCH;
    }

    total_sectors = expected_bytes / INSTALLER_SECTOR_SIZE;
    sha256_init(&sha);
    for (i = 0u; i < total_sectors; i++) {
        u32 got = 0u;
        fat_chain_status_t st = FAT_CHAIN_ERR_ARG;

        if (!fat_read_file_range(vol, &entry, i * INSTALLER_SECTOR_SIZE,
                                 s_sd_verify_sector, INSTALLER_SECTOR_SIZE,
                                 &got, &st) ||
            st != FAT_CHAIN_OK || got != INSTALLER_SECTOR_SIZE) {
            return INSTALLER_ERR_BACKUP_FAILED;
        }
        sha256_update(&sha, s_sd_verify_sector, INSTALLER_SECTOR_SIZE);
        installer_progress_step(INSTALLER_STATE_VERIFY_BACKUP, i + 1u,
                                total_sectors, "verify SD backup");
    }
    sha256_final(&sha, digest);
    if (out != NULL) {
        dsi_memcpy(out->digest, digest, SHA256_DIGEST_SIZE);
    }

    if (!sha256_digest_equal(digest, expected)) {
        return INSTALLER_ERR_BACKUP_MISMATCH;
    }

    if (out != NULL) {
        out->ok = true;
        out->sectors = total_sectors;
        out->bytes = expected_bytes;
        out->error = INSTALLER_ERR_NONE;
    }
    return INSTALLER_ERR_NONE;
}

/* ---- FAT-file source adapter (read-only, always compiled) ------------- */

void installer_sd_source_init(installer_sd_source_t *s, fat_volume_t *vol,
                              const fat_file_entry_t *entry)
{
    if (s == NULL) {
        return;
    }
    s->vol = vol;
    if (entry != NULL) {
        s->entry = *entry;
    } else {
        dsi_memset(&s->entry, 0, sizeof(s->entry));
    }
}

bool installer_sd_source_read(void *ctx, u32 offset, u8 *out, u32 len)
{
    installer_sd_source_t *s = (installer_sd_source_t *)ctx;
    u32 got = 0u;
    fat_chain_status_t status = FAT_CHAIN_ERR_ARG;

    if (s == NULL || out == NULL || len == 0u) {
        return false;
    }
    dsi_memset(out, 0, len);

    (void)fat_read_file_range(s->vol, &s->entry, offset, out, len, &got,
                              &status);
    if (status == FAT_CHAIN_ERR_IO || s->vol->io_error) {
        return false;
    }
    /*
     * A short read past EOF is expected: the flash engine zero-pads the tail.
     * Only a hard I/O error fails the transfer.
     */
    return true;
}

/* ---- SD backup wrapper (gated) ---------------------------------------- */

#ifdef ENABLE_NAND_WRITE_DANGER

#include "arm9/storage.h"
#include "arm9/nand_boot.h"
#include "common/ipc_protocol.h"

/* Sector-write adapter: persists one 512-byte sector to the SD card. */
static bool installer_sd_write_sector(void *ctx, u32 lba,
                                      const u8 sector[INSTALLER_SECTOR_SIZE])
{
    (void)ctx;
    return arm9_storage_write_sectors(IPC_STORAGE_DEV_SD, lba, sector, 1u);
}

/* eMMC read adapter for the backup stream. */
static bool installer_nand_read_sector(void *ctx, u32 lba,
                                       u8 out[INSTALLER_SECTOR_SIZE])
{
    (void)ctx;
    return arm9_nand_read_sectors(lba, out, 1u);
}

/* Sink adapter: append to the open FAT writer. */
static bool installer_fat_sink(void *ctx, const u8 *data, u32 len)
{
    return fat_writer_write((fat_writer_t *)ctx, data, len) == FAT_WR_OK;
}

installer_error_t installer_backup_to_sd(fat_volume_t *vol,
                                         u32 start_lba, u32 sectors,
                                         installer_transfer_t *out)
{
    static fat_writer_t writer;
    fat_write_status_t st;
    installer_error_t err;

    if (vol == NULL || sectors == 0u) {
        return INSTALLER_ERR_BACKUP_FAILED;
    }

    st = fat_create_file(vol, installer_sd_write_sector, NULL,
                         INSTALLER_BACKUP_FILE, &writer);
    if (st != FAT_WR_OK) {
        /* Never clobber an existing backup: report and refuse. */
        return INSTALLER_ERR_BACKUP_FAILED;
    }

    err = installer_backup_stream(installer_nand_read_sector, NULL,
                                  installer_fat_sink, &writer,
                                  start_lba, sectors, out);
    if (err != INSTALLER_ERR_NONE) {
        fat_writer_abort(&writer);   /* frees the partial file's clusters */
        return err;
    }

    st = fat_writer_end(&writer, NULL);
    if (st != FAT_WR_OK) {
        fat_writer_abort(&writer);
        return INSTALLER_ERR_BACKUP_FAILED;
    }
    return INSTALLER_ERR_NONE;
}

#endif /* ENABLE_NAND_WRITE_DANGER */
