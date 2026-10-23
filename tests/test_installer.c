/*
 * tests/test_installer.c
 *
 * Host-side unit/integration tests for the NAND installer subsystem:
 *
 *   suite 14a  SHA-256 / HMAC-SHA-256 known-answer vectors
 *   suite 14b  NAND layout validation + protected-region guards
 *   suite 14c  signed installer token codec + pre-flight policy + state machine
 *   suite 14d  streaming backup + verify engines (injected I/O)
 *   suite 14e  flash + read-back engine (injected I/O)
 *   suite 14f  bounded FAT16/FAT32 root-file writer
 *   suite 14g  installer_run() locked-build reporting
 *   suite 14h  progress hook + pure percentage helper
 *   suite 14i  simulated menu [10] pre-flight with no INSTALL.KEY
 *
 * Build/run with:  make host-test   (passes build/sdcard.img as argv[1] so the
 * FAT writer can be exercised against a real volume)
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/sha256.h"
#include "common/installer.h"
#include "common/nand.h"
#include "common/fat_ro.h"
#include "common/fat_rw.h"
#include "common/mem.h"
#include "arm9/installer.h"

/* ------------------------------------------------------------------ */
/* Little-endian helpers (mirrors tests/test_nand.c)                   */
/* ------------------------------------------------------------------ */

static void put32(u8 *p, u32 off, u32 v)
{
    p[off] = (u8)(v & 0xFFu);
    p[off + 1u] = (u8)((v >> 8) & 0xFFu);
    p[off + 2u] = (u8)((v >> 16) & 0xFFu);
    p[off + 3u] = (u8)((v >> 24) & 0xFFu);
}

static void set_sig(u8 *sector)
{
    sector[NAND_MBR_SIG_OFF] = 0x55u;
    sector[NAND_MBR_SIG_OFF + 1u] = 0xAAu;
}

static void set_pc_entry(u8 *sector, u32 i, u8 type, u32 lba, u32 count)
{
    u8 *e = sector + NAND_MBR_TABLE_OFF + i * NAND_MBR_ENTRY_SIZE;

    memset(e, 0, NAND_MBR_ENTRY_SIZE);
    e[NAND_MBR_TYPE_OFF] = type;
    put32(e, NAND_MBR_LBA_OFF, lba);
    put32(e, NAND_MBR_COUNT_OFF, count);
}

static void hex_of(const u8 *digest, char out[SHA256_HEX_SIZE])
{
    sha256_hex(digest, out);
}

/* ------------------------------------------------------------------ */
/* 14a  SHA-256 / HMAC known-answer vectors                            */
/* ------------------------------------------------------------------ */

static void test_sha256_kat(void)
{
    u8 d[SHA256_DIGEST_SIZE];
    char hex[SHA256_HEX_SIZE];
    sha256_ctx_t ctx;

    sha256("", 0u, d);
    hex_of(d, hex);
    assert(strcmp(hex,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
        == 0);

    sha256("abc", 3u, d);
    hex_of(d, hex);
    assert(strcmp(hex,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        == 0);

    {
        const char *s =
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        sha256(s, (u32)strlen(s), d);
        hex_of(d, hex);
        assert(strcmp(hex,
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1")
            == 0);
    }

    /* Incremental updates must match the one-shot result. */
    sha256_init(&ctx);
    sha256_update(&ctx, "ab", 2u);
    sha256_update(&ctx, "", 0u);
    sha256_update(&ctx, "c", 1u);
    sha256_final(&ctx, d);
    hex_of(d, hex);
    assert(strcmp(hex,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        == 0);

    /* NULL safety. */
    sha256_init(NULL);
    sha256_update(NULL, "x", 1u);
    sha256_final(NULL, d);
    sha256_hex(NULL, hex);
}

static void test_hmac_kat(void)
{
    /* RFC 4231 test case 1. */
    u8 key[20];
    u8 out[SHA256_DIGEST_SIZE];
    char hex[SHA256_HEX_SIZE];

    memset(key, 0x0B, sizeof(key));
    hmac_sha256(key, sizeof(key), "Hi There", 8u, out);
    hex_of(out, hex);
    assert(strcmp(hex,
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7")
        == 0);

    /* Incremental HMAC must match. */
    {
        hmac_sha256_ctx_t ctx;
        u8 out2[SHA256_DIGEST_SIZE];

        hmac_sha256_init(&ctx, key, sizeof(key));
        hmac_sha256_update(&ctx, "Hi ", 3u);
        hmac_sha256_update(&ctx, "There", 5u);
        hmac_sha256_final(&ctx, out2);
        assert(sha256_digest_equal(out, out2));
    }

    /* Constant-time comparison semantics. */
    {
        u8 a[SHA256_DIGEST_SIZE];
        u8 b[SHA256_DIGEST_SIZE];

        memcpy(a, out, sizeof(a));
        memcpy(b, out, sizeof(b));
        assert(sha256_digest_equal(a, b));
        b[0] ^= 0x01u;
        assert(!sha256_digest_equal(a, b));
    }
}

/* ------------------------------------------------------------------ */
/* 14b  NAND layout validation                                         */
/* ------------------------------------------------------------------ */

static void test_nand_layout(void)
{
    u8 sector[NAND_SECTOR_SIZE];
    nand_layout_t layout;

    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x2000u);

    assert(nand_validate_layout(sector, NAND_MAX_SECTORS, &layout) ==
           NAND_LAYOUT_OK);
    assert(layout.format == NAND_TABLE_PC_MBR);
    assert(layout.partition_lba == 0x100u);
    assert(layout.partition_sectors == 0x2000u);
    assert(layout.protected_end == 0x100u);
    assert(layout.total_sectors == 0x2100u);
    assert(layout.total_backup_sectors == 0x2100u);

    /* The boot prefix [0, 0x100) is protected; the volume itself is not. */
    assert(nand_region_is_protected(&layout, 0u, 1u));
    assert(nand_region_is_protected(&layout, 0xFFu, 1u));
    assert(!nand_region_is_protected(&layout, 0x100u, 1u));
    assert(!nand_region_is_protected(&layout, 0x2000u, 0x100u));
    /* Fail closed on a zero length or overflow. */
    assert(nand_region_is_protected(&layout, 0x100u, 0u));
    assert(nand_region_is_protected(&layout, 0xFFFFFFF0u, 0x20u));
    assert(nand_region_is_protected(NULL, 0x100u, 1u));

    /* Signature / table errors. */
    {
        u8 bad[NAND_SECTOR_SIZE];

        memset(bad, 0, sizeof(bad));
        assert(nand_validate_layout(bad, NAND_MAX_SECTORS, &layout) ==
               NAND_LAYOUT_ERR_SIGNATURE);

        set_sig(bad);
        assert(nand_validate_layout(bad, NAND_MAX_SECTORS, &layout) ==
               NAND_LAYOUT_ERR_EMPTY);
    }

    /* A typed-but-unknown partition with no FAT and no TWL entry is rejected.
     * (Type 0 keeps the TWL interpretation empty so the PC path is taken.) */
    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, 0x00u, 0x100u, 0x2000u);
    assert(nand_validate_layout(sector, NAND_MAX_SECTORS, &layout) ==
           NAND_LAYOUT_ERR_NO_FAT);

    /* A FAT entry at LBA 0 is not a usable volume: the auto-detector reads it
     * as a degenerate TWL entry, and the size check rejects it. Either way the
     * layout is refused (fail closed). */
    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0u, 0x2000u);
    assert(nand_validate_layout(sector, NAND_MAX_SECTORS, &layout) !=
           NAND_LAYOUT_OK);

    /* Too small. */
    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x10u);
    assert(nand_validate_layout(sector, NAND_MAX_SECTORS, &layout) ==
           NAND_LAYOUT_ERR_TOO_SMALL);

    /* Out of bounds. */
    memset(sector, 0, sizeof(sector));
    set_sig(sector);
    set_pc_entry(sector, 0u, FAT_PART_TYPE_FAT16, 0x100u, 0x2000u);
    assert(nand_validate_layout(sector, 0x1000u, &layout) ==
           NAND_LAYOUT_ERR_BOUNDS);

    /* NULL safety. */
    assert(nand_validate_layout(NULL, NAND_MAX_SECTORS, &layout) ==
           NAND_LAYOUT_ERR_NULL);
    assert(nand_validate_layout(sector, NAND_MAX_SECTORS, NULL) ==
           NAND_LAYOUT_ERR_NULL);
    assert(nand_layout_status_text(NAND_LAYOUT_OK) != NULL);
    assert(nand_layout_status_text((nand_layout_status_t)99) != NULL);
}

/* ------------------------------------------------------------------ */
/* 14c  Token codec + policy + state machine                           */
/* ------------------------------------------------------------------ */

static void test_token(void)
{
    installer_token_t t;
    installer_token_t parsed;
    u8 buf[INSTALLER_TOKEN_SIZE];
    u8 payload[4096];

    memset(&t, 0, sizeof(t));
    memset(payload, 0x5A, sizeof(payload));
    t.magic = INSTALLER_TOKEN_MAGIC;
    t.version = INSTALLER_TOKEN_VERSION;
    t.target_lba = 0x100u;
    t.target_sectors = 8u;
    t.payload_sectors = 8u;
    sha256(payload, 8u * 512u, t.payload_sha256);

    installer_token_encode(&t, NULL, 0u, buf);
    assert(installer_token_parse(buf, sizeof(buf), &parsed) ==
           INSTALLER_TOKEN_OK);
    assert(parsed.target_lba == 0x100u);
    assert(parsed.target_sectors == 8u);
    assert(sha256_digest_equal(parsed.payload_sha256, t.payload_sha256));
    assert(installer_token_verify(&parsed, NULL, 0u) == INSTALLER_TOKEN_OK);

    /* Tampering with a signed field breaks authentication. */
    {
        u8 tampered[INSTALLER_TOKEN_SIZE];
        installer_token_t tp;

        memcpy(tampered, buf, sizeof(tampered));
        tampered[0x08] ^= 0x01u;   /* target_lba */
        assert(installer_token_parse(tampered, sizeof(tampered), &tp) ==
               INSTALLER_TOKEN_OK);
        assert(installer_token_verify(&tp, NULL, 0u) ==
               INSTALLER_TOKEN_ERR_SIGNATURE);
    }

    /* Structural rejection. */
    {
        u8 bad[INSTALLER_TOKEN_SIZE];

        memcpy(bad, buf, sizeof(bad));
        assert(installer_token_parse(bad, sizeof(bad) - 1u, &parsed) ==
               INSTALLER_TOKEN_ERR_SIZE);
        bad[0] ^= 0xFFu;
        assert(installer_token_parse(bad, sizeof(bad), &parsed) ==
               INSTALLER_TOKEN_ERR_MAGIC);

        memcpy(bad, buf, sizeof(bad));
        bad[0x04] = 0x09u;   /* version */
        assert(installer_token_parse(bad, sizeof(bad), &parsed) ==
               INSTALLER_TOKEN_ERR_VERSION);

        memcpy(bad, buf, sizeof(bad));
        bad[0x34] = 0x01u;   /* reserved */
        assert(installer_token_parse(bad, sizeof(bad), &parsed) ==
               INSTALLER_TOKEN_ERR_RANGE);

        memcpy(bad, buf, sizeof(bad));
        put32(bad, 0x0C, 0u);   /* target_sectors == 0 */
        assert(installer_token_parse(bad, sizeof(bad), &parsed) ==
               INSTALLER_TOKEN_ERR_RANGE);
    }

    assert(installer_token_parse(NULL, sizeof(buf), &parsed) ==
           INSTALLER_TOKEN_ERR_NULL);
    assert(installer_token_parse(buf, sizeof(buf), NULL) ==
           INSTALLER_TOKEN_ERR_NULL);
    assert(installer_token_verify(NULL, NULL, 0u) ==
           INSTALLER_TOKEN_ERR_NULL);
    assert(installer_token_status_text(INSTALLER_TOKEN_OK) != NULL);
    assert(installer_token_status_text((installer_token_status_t)99) != NULL);
}

static void test_policy(void)
{
    nand_layout_t layout;
    installer_token_t t;
    installer_region_t region;
    installer_request_t req;
    installer_decision_t d;

    memset(&layout, 0, sizeof(layout));
    layout.format = NAND_TABLE_PC_MBR;
    layout.partition_lba = 0x100u;
    layout.partition_sectors = 0x2000u;
    layout.total_sectors = 0x2100u;
    layout.protected_end = 0x100u;

    memset(&t, 0, sizeof(t));
    t.magic = INSTALLER_TOKEN_MAGIC;
    t.version = INSTALLER_TOKEN_VERSION;
    t.target_lba = 0x100u;
    t.target_sectors = 8u;
    t.payload_sectors = 8u;

    assert(installer_token_check_range(&t, &layout) == INSTALLER_TOKEN_OK);
    assert(installer_plan_region(&t, &layout, &region) == INSTALLER_ERR_NONE);
    assert(region.lba == 0x100u && region.count == 8u);

    /* Target in the protected prefix. */
    t.target_lba = 0u;
    assert(installer_token_check_range(&t, &layout) ==
           INSTALLER_TOKEN_ERR_RANGE);
    assert(installer_plan_region(&t, &layout, &region) ==
           INSTALLER_ERR_TARGET_RANGE);

    /* Target past the volume end. */
    t.target_lba = 0x2100u;
    assert(installer_token_check_range(&t, &layout) ==
           INSTALLER_TOKEN_ERR_RANGE);

    /* payload/target mismatch is refused by the plan. */
    t.target_lba = 0x100u;
    t.target_sectors = 8u;
    t.payload_sectors = 4u;
    assert(installer_plan_region(&t, &layout, &region) ==
           INSTALLER_ERR_TARGET_RANGE);

    assert(installer_plan_region(NULL, &layout, &region) ==
           INSTALLER_ERR_BAD_TOKEN);
    assert(installer_plan_region(&t, NULL, &region) ==
           INSTALLER_ERR_BAD_TOKEN);
    assert(installer_plan_region(&t, &layout, NULL) ==
           INSTALLER_ERR_BAD_TOKEN);

    /* Pre-flight cascade. */
    memset(&req, 0, sizeof(req));
    req.write_enabled = true;
    req.sd_present = true;
    req.token_valid = true;
    req.layout_ok = true;
    d = installer_preflight(&req);
    assert(d.proceed && d.next_state == INSTALLER_STATE_BACKUP);
    assert(d.error == INSTALLER_ERR_NONE);

    req.backup_verified = true;
    d = installer_preflight(&req);
    assert(d.proceed && d.next_state == INSTALLER_STATE_UNLOCK);

    /* A retail, AES-CTR encrypted image is refused with a specific status that
     * the UI can explain, and it outranks the layout/token checks. */
    req.retail_encrypted = true;
    d = installer_preflight(&req);
    assert(!d.proceed && d.error == INSTALLER_ERR_ENCRYPTED_NAND);
    assert(strcmp(installer_error_text(INSTALLER_ERR_ENCRYPTED_NAND),
                  "retail encrypted NAND (AES/TMD required)") == 0);
    req.retail_encrypted = false;

    /*
     * The opt-in retail raw-write path proceeds only when explicitly armed
     * (compile flag + retail token + runtime confirmation): it then bypasses
     * the (unreadable) layout check but is still bounded by the retail boot
     * guard computed at the call site.
     */
    req.retail_encrypted = true;
    req.retail_write_armed = true;
    req.layout_ok = false;
    req.backup_verified = false;
    d = installer_preflight(&req);
    assert(d.proceed && d.next_state == INSTALLER_STATE_BACKUP);
    req.retail_write_armed = false;
    req.retail_encrypted = false;
    req.layout_ok = true;

    req.write_enabled = false;
    d = installer_preflight(&req);
    assert(!d.proceed && d.error == INSTALLER_ERR_DISABLED);

    req.write_enabled = true;
    req.sd_present = false;
    d = installer_preflight(&req);
    assert(!d.proceed && d.error == INSTALLER_ERR_NO_SD);

    req.sd_present = true;
    req.layout_ok = false;
    d = installer_preflight(&req);
    assert(!d.proceed && d.error == INSTALLER_ERR_BAD_LAYOUT);

    req.layout_ok = true;
    req.token_valid = false;
    d = installer_preflight(&req);
    assert(!d.proceed && d.error == INSTALLER_ERR_BAD_TOKEN);

    d = installer_preflight(NULL);
    assert(!d.proceed);

    assert(installer_error_text(INSTALLER_ERR_NONE) != NULL);
    assert(installer_error_text((installer_error_t)99) != NULL);
}

static void test_state_machine(void)
{
    assert(installer_state_text(INSTALLER_STATE_DONE) != NULL);
    assert(installer_state_text((installer_state_t)99) != NULL);

    assert(installer_advance(INSTALLER_STATE_IDLE, true) ==
           INSTALLER_STATE_PREFLIGHT);
    assert(installer_advance(INSTALLER_STATE_PREFLIGHT, true) ==
           INSTALLER_STATE_BACKUP);
    assert(installer_advance(INSTALLER_STATE_BACKUP, true) ==
           INSTALLER_STATE_VERIFY_BACKUP);
    assert(installer_advance(INSTALLER_STATE_VERIFY_BACKUP, true) ==
           INSTALLER_STATE_UNLOCK);
    assert(installer_advance(INSTALLER_STATE_UNLOCK, true) ==
           INSTALLER_STATE_WRITE);
    assert(installer_advance(INSTALLER_STATE_WRITE, true) ==
           INSTALLER_STATE_VERIFY_WRITE);
    assert(installer_advance(INSTALLER_STATE_VERIFY_WRITE, true) ==
           INSTALLER_STATE_DONE);
    assert(installer_advance(INSTALLER_STATE_DONE, true) ==
           INSTALLER_STATE_DONE);
    assert(installer_advance(INSTALLER_STATE_DONE, false) ==
           INSTALLER_STATE_ABORTED);

    /* A failure aborts from any active state, but IDLE stays idle. */
    assert(installer_advance(INSTALLER_STATE_BACKUP, false) ==
           INSTALLER_STATE_ABORTED);
    assert(installer_advance(INSTALLER_STATE_IDLE, false) ==
           INSTALLER_STATE_IDLE);
}

/* ------------------------------------------------------------------ */
/* 14d/e  Injected-I/O backup + flash engines                          */
/* ------------------------------------------------------------------ */

#define TEST_DEV_SECTORS 0x100u

typedef struct {
    u8  data[TEST_DEV_SECTORS * 512u];
    u32 sectors;
} mem_dev_t;

static bool mem_dev_read(void *ctx, u32 lba, u8 out[512])
{
    mem_dev_t *d = (mem_dev_t *)ctx;

    if (d == NULL || lba >= d->sectors) {
        return false;
    }
    memcpy(out, d->data + (size_t)lba * 512u, 512u);
    return true;
}

/* A write callback that silently drops the data (read-back must diverge). */
static bool mem_dev_drop_write(void *ctx, u32 lba, const u8 sector[512])
{
    (void)ctx;
    (void)lba;
    (void)sector;
    return true;
}

static bool mem_dev_write(void *ctx, u32 lba, const u8 sector[512])
{
    mem_dev_t *d = (mem_dev_t *)ctx;

    if (d == NULL || lba >= d->sectors) {
        return false;
    }
    memcpy(d->data + (size_t)lba * 512u, sector, 512u);
    return true;
}

typedef struct {
    u8  data[TEST_DEV_SECTORS * 512u];
    u32 len;
} mem_sink_t;

static bool mem_sink_write(void *ctx, const u8 *data, u32 len)
{
    mem_sink_t *s = (mem_sink_t *)ctx;

    if (s == NULL || s->len + len > sizeof(s->data)) {
        return false;
    }
    memcpy(s->data + s->len, data, len);
    s->len += len;
    return true;
}

typedef struct {
    const u8 *data;
    u32 len;
} mem_src_t;

static bool mem_src_read(void *ctx, u32 offset, u8 *out, u32 len)
{
    mem_src_t *s = (mem_src_t *)ctx;

    memset(out, 0, len);
    if (s != NULL && offset < s->len) {
        u32 n = s->len - offset;

        if (n > len) {
            n = len;
        }
        memcpy(out, s->data + offset, n);
    }
    return true;
}

static void test_backup_flash(void)
{
    static mem_dev_t dev;
    static mem_sink_t sink;
    static mem_dev_t target;
    static mem_src_t src;
    installer_transfer_t xfer;
    u8 expect[SHA256_DIGEST_SIZE];
    u32 i;

    for (i = 0u; i < sizeof(dev.data); i++) {
        dev.data[i] = (u8)((i * 31u + 7u) & 0xFFu);
    }
    dev.sectors = TEST_DEV_SECTORS;

    /* Backup stream: hash + sink must agree with the source. */
    assert(installer_backup_stream(mem_dev_read, &dev, mem_sink_write, &sink,
                                   0u, 0x20u, &xfer) == INSTALLER_ERR_NONE);
    assert(xfer.ok && xfer.sectors == 0x20u);
    assert(xfer.bytes == 0x20u * 512u);
    assert(sink.len == xfer.bytes);
    assert(memcmp(sink.data, dev.data, xfer.bytes) == 0);

    sha256(dev.data, 0x20u * 512u, expect);
    assert(sha256_digest_equal(xfer.digest, expect));

    /* Verification against the live source. */
    assert(installer_backup_verify(mem_dev_read, &dev, xfer.digest, 0u, 0x20u)
           == INSTALLER_ERR_NONE);

    expect[0] ^= 0xFFu;
    assert(installer_backup_verify(mem_dev_read, &dev, expect, 0u, 0x20u) ==
           INSTALLER_ERR_BACKUP_MISMATCH);

    /* A read failure aborts the stream. */
    assert(installer_backup_stream(mem_dev_read, &dev, mem_sink_write, &sink,
                                   0x200u, 0x20u, &xfer) ==
           INSTALLER_ERR_BACKUP_FAILED);
    assert(!xfer.ok);

    /* Failure arguments. */
    assert(installer_backup_stream(NULL, &dev, mem_sink_write, &sink, 0u, 1u,
                                   &xfer) == INSTALLER_ERR_BACKUP_FAILED);
    assert(installer_backup_stream(mem_dev_read, &dev, NULL, &sink, 0u, 1u,
                                   &xfer) == INSTALLER_ERR_BACKUP_FAILED);
    assert(installer_backup_stream(mem_dev_read, &dev, mem_sink_write, &sink,
                                   0u, 0u, &xfer) == INSTALLER_ERR_BACKUP_FAILED);

    /* Flash: source -> target with read-back verification. */
    {
        memset(&target, 0, sizeof(target));
        target.sectors = TEST_DEV_SECTORS;
        src.data = dev.data;
        src.len = 0x20u * 512u;

        sha256(dev.data, 0x20u * 512u, expect);
        assert(installer_flash_region(mem_dev_read, &target,
                                      mem_dev_write, &target,
                                      mem_src_read, &src,
                                      0x40u, 0x20u, expect, &xfer) ==
               INSTALLER_ERR_NONE);
        assert(xfer.ok && sha256_digest_equal(xfer.digest, expect));
        assert(memcmp(target.data + 0x40u * 512u, dev.data, 0x20u * 512u) == 0);

        /* Wrong expected digest is reported. */
        expect[0] ^= 0x01u;
        assert(installer_flash_region(mem_dev_read, &target,
                                      mem_dev_write, &target,
                                      mem_src_read, &src,
                                      0x40u, 0x20u, expect, &xfer) ==
               INSTALLER_ERR_VERIFY_MISMATCH);

        /* A dropped write makes the read-back diverge. */
        {
            static mem_dev_t ro;

            memset(&ro, 0, sizeof(ro));
            ro.sectors = TEST_DEV_SECTORS;
            assert(installer_flash_region(mem_dev_read, &ro,
                                          mem_dev_drop_write, &ro,
                                          mem_src_read, &src,
                                          0u, 4u, NULL, &xfer) ==
                   INSTALLER_ERR_VERIFY_MISMATCH);
        }
    }

    /* Flash failure arguments. */
    assert(installer_flash_region(NULL, &target, mem_dev_write, &target,
                                  mem_src_read, &src, 0u, 1u, NULL, &xfer) ==
           INSTALLER_ERR_WRITE_FAILED);
    assert(installer_flash_region(mem_dev_read, &target, NULL, &target,
                                  mem_src_read, &src, 0u, 1u, NULL, &xfer) ==
           INSTALLER_ERR_WRITE_FAILED);
    assert(installer_flash_region(mem_dev_read, &target, mem_dev_write, &target,
                                  NULL, &src, 0u, 1u, NULL, &xfer) ==
           INSTALLER_ERR_WRITE_FAILED);
    assert(installer_flash_region(mem_dev_read, &target, mem_dev_write, &target,
                                  mem_src_read, &src, 0u, 0u, NULL, &xfer) ==
           INSTALLER_ERR_WRITE_FAILED);
}

/* ------------------------------------------------------------------ */
/* 14f  FAT writer against the generated build/sdcard.img              */
/* ------------------------------------------------------------------ */

static u8 *g_img;
static u32 g_img_sectors;

/* Load the generated SD image into g_img; false when it is unavailable. */
static bool img_load(const char *path)
{
    FILE *fh = fopen(path, "rb");
    long size;

    if (fh == NULL) {
        return false;
    }
    fseek(fh, 0L, SEEK_END);
    size = ftell(fh);
    fseek(fh, 0L, SEEK_SET);
    assert(size > 0);
    g_img = (u8 *)malloc((size_t)size);
    assert(g_img != NULL);
    assert(fread(g_img, 1u, (size_t)size, fh) == (size_t)size);
    fclose(fh);
    g_img_sectors = (u32)size / FAT_SECTOR_SIZE;
    return true;
}

static bool img_read(void *ctx, u32 lba, u8 out[FAT_SECTOR_SIZE])
{
    (void)ctx;
    if (lba >= g_img_sectors) {
        return false;
    }
    memcpy(out, g_img + (size_t)lba * FAT_SECTOR_SIZE, FAT_SECTOR_SIZE);
    return true;
}

static bool img_write(void *ctx, u32 lba, const u8 sector[FAT_SECTOR_SIZE])
{
    (void)ctx;
    if (lba >= g_img_sectors) {
        return false;
    }
    memcpy(g_img + (size_t)lba * FAT_SECTOR_SIZE, sector, FAT_SECTOR_SIZE);
    return true;
}

/* Reports success but silently drops every sector, like a read-only emulated
 * SD card: the file never actually appears on the volume. */
static bool img_drop_write(void *ctx, u32 lba, const u8 sector[FAT_SECTOR_SIZE])
{
    (void)ctx;
    (void)lba;
    (void)sector;
    return true;
}

static void test_fat_writer(const char *path)
{
    static fat_volume_t vol;
    static fat_writer_t w;
    static installer_sd_source_t src;
    static u8 pattern[13312];   /* 26 sectors: sector-aligned for the backup */
    static u8 readback[13312];
    fat_file_entry_t e;
    installer_transfer_t out;
    u8 digest[SHA256_DIGEST_SIZE];
    u32 got = 0u;
    u32 out_size = 0u;
    u32 i;
    fat_chain_status_t status;

    if (!img_load(path)) {
        printf("test_installer: skipping FAT writer test (%s not found)\n",
               path);
        return;
    }

    assert(fat_mount(&vol, img_read, NULL));

    /* Name validation. */
    assert(fat_create_file(&vol, img_write, NULL, "BAD NAME.BIN", &w) ==
           FAT_WR_ERR_NAME);
    assert(fat_create_file(&vol, img_write, NULL, "TOOLONGNAME.BIN", &w) ==
           FAT_WR_ERR_NAME);
    assert(fat_create_file(&vol, img_write, NULL, ".BIN", &w) ==
           FAT_WR_ERR_NAME);
    assert(fat_create_file(NULL, img_write, NULL, "X.BIN", &w) ==
           FAT_WR_ERR_ARG);

    /* Create + multi-cluster sequential write. */
    assert(fat_create_file(&vol, img_write, NULL, "NANDBACK.BIN", &w) ==
           FAT_WR_OK);
    for (i = 0u; i < sizeof(pattern); i++) {
        pattern[i] = (u8)((i * 7u + 3u) & 0xFFu);
    }
    assert(fat_writer_write(&w, pattern, sizeof(pattern)) == FAT_WR_OK);
    assert(fat_writer_end(&w, &out_size) == FAT_WR_OK);
    assert(out_size == sizeof(pattern));

    /* A duplicate name must fail closed (never clobber a backup). */
    assert(fat_create_file(&vol, img_write, NULL, "nandback.bin", &w) ==
           FAT_WR_ERR_EXISTS);

    /* Re-mount and read the file back byte-for-byte. */
    assert(fat_mount(&vol, img_read, NULL));
    assert(fat_find_path(&vol, "NANDBACK.BIN", &e));
    assert(!e.is_dir);
    assert(e.file_size == sizeof(pattern));
    assert(fat_read_file_range(&vol, &e, 0u, readback, sizeof(readback),
                               &got, &status));
    assert(status == FAT_CHAIN_OK);
    assert(got == sizeof(pattern));
    assert(memcmp(readback, pattern, sizeof(pattern)) == 0);

    /*
     * SD persistence verification: the file we just wrote must hash to the
     * source digest, and a wrong digest or a wrong size must be rejected.
     */
    sha256(pattern, sizeof(pattern), digest);
    assert(installer_backup_verify_sd(&vol, digest, sizeof(pattern), &out) ==
           INSTALLER_ERR_NONE);
    assert(out.ok && out.bytes == sizeof(pattern) &&
           out.error == INSTALLER_ERR_NONE);
    assert(sha256_digest_equal(out.digest, digest));

    digest[0] ^= 0xFFu;
    assert(installer_backup_verify_sd(&vol, digest, sizeof(pattern), NULL) ==
           INSTALLER_ERR_BACKUP_MISMATCH);

    sha256(pattern, sizeof(pattern), digest);
    assert(installer_backup_verify_sd(&vol, digest,
                                      sizeof(pattern) + 512u, NULL) ==
           INSTALLER_ERR_BACKUP_MISMATCH);

    /*
     * The read-only installer source adapter must stream the same bytes and
     * zero-pad a read past EOF.
     */
    installer_sd_source_init(&src, &vol, &e);
    memset(readback, 0xAA, sizeof(readback));
    assert(installer_sd_source_read(&src, 0u, readback, 512u));
    assert(memcmp(readback, pattern, 512u) == 0);
    memset(readback, 0xAA, sizeof(readback));
    assert(installer_sd_source_read(&src, sizeof(pattern) - 4u, readback,
                                    512u));
    assert(memcmp(readback, pattern + sizeof(pattern) - 4u, 4u) == 0);
    assert(readback[4] == 0x00u && readback[511] == 0x00u);

    free(g_img);
    g_img = NULL;
}

/* Re-open NANDBACK.BIN through the FAT reader and prove persistence. */
static void test_backup_verify_sd(const char *path)
{
    static fat_volume_t vol;
    static fat_writer_t w;
    static u8 pattern[13312];   /* 26 sectors: sector-aligned for the backup */
    installer_transfer_t out;
    u8 digest[SHA256_DIGEST_SIZE];
    u32 i;

    if (!img_load(path)) {
        printf("test_installer: skipping SD verify test (%s not found)\n",
               path);
        return;
    }
    assert(fat_mount(&vol, img_read, NULL));

    /* A missing NANDBACK.BIN is a persistence failure, not a success. */
    sha256(pattern, 512u, digest);
    assert(installer_backup_verify_sd(&vol, digest, 512u, &out) ==
           INSTALLER_ERR_BACKUP_MISMATCH);
    assert(!out.ok);

    /* Argument safety. */
    assert(installer_backup_verify_sd(NULL, digest, 512u, NULL) ==
           INSTALLER_ERR_BACKUP_FAILED);
    assert(installer_backup_verify_sd(&vol, NULL, 512u, NULL) ==
           INSTALLER_ERR_BACKUP_FAILED);
    assert(installer_backup_verify_sd(&vol, digest, 0u, NULL) ==
           INSTALLER_ERR_BACKUP_FAILED);
    assert(installer_backup_verify_sd(&vol, digest, 511u, NULL) ==
           INSTALLER_ERR_BACKUP_FAILED);

    /*
     * Simulate a read-only emulated SD card: every write reports success but is
     * dropped, so NANDBACK.BIN never actually appears. The SD read-back verify
     * must catch it deterministically (this is the melonDS ReadOnly case).
     */
    for (i = 0u; i < sizeof(pattern); i++) {
        pattern[i] = (u8)((i * 13u + 5u) & 0xFFu);
    }
    assert(fat_create_file(&vol, img_drop_write, NULL, INSTALLER_BACKUP_FILE,
                           &w) == FAT_WR_OK);
    assert(fat_writer_write(&w, pattern, sizeof(pattern)) == FAT_WR_OK);
    assert(fat_writer_end(&w, NULL) == FAT_WR_OK);
    sha256(pattern, sizeof(pattern), digest);
    assert(installer_backup_verify_sd(&vol, digest, sizeof(pattern), NULL) ==
           INSTALLER_ERR_BACKUP_MISMATCH);

    free(g_img);
    g_img = NULL;
}

static void test_fat_writer_reject(void)
{
    /* An unmounted volume is refused outright. */
    fat_volume_t vol;
    fat_writer_t w;

    memset(&vol, 0, sizeof(vol));
    vol.type = FAT_TYPE_NONE;
    assert(fat_create_file(&vol, img_write, NULL, "X.BIN", &w) ==
           FAT_WR_ERR_ARG);
    assert(fat_writer_write(NULL, NULL, 0u) == FAT_WR_ERR_ARG);
    assert(fat_writer_end(NULL, NULL) == FAT_WR_ERR_ARG);
    fat_writer_abort(NULL);
    assert(fat_write_status_text(FAT_WR_OK) != NULL);
    assert(fat_write_status_text((fat_write_status_t)99) != NULL);
}

/* ------------------------------------------------------------------ */
/* 14i  Simulated menu [10] pre-flight with no INSTALL.KEY             */
/* ------------------------------------------------------------------ */

/*
 * Smoke-simulate what menu item [10] must do with no installer package on the
 * SD card: detect that INSTALL.KEY is absent, refuse before touching a single
 * eMMC sector and report the abort so the UI can return to the main menu.
 */
static void test_no_token_preflight(const char *path)
{
    static fat_volume_t vol;
    fat_file_entry_t e;
    installer_request_t req;
    installer_decision_t d;

    if (!img_load(path)) {
        printf("test_installer: skipping no-token test (%s not found)\n",
               path);
        return;
    }
    assert(fat_mount(&vol, img_read, NULL));

    /* The generated SD image deliberately ships no INSTALL.KEY. */
    assert(!fat_find_path(&vol, INSTALLER_TOKEN_FILE, &e));

    /* Mirror installer_run()'s pre-flight inputs for that case (armed build). */
    memset(&req, 0, sizeof(req));
    req.write_enabled = true;
    req.sd_present = true;
    req.layout_ok = true;
    req.token_valid = false;
    req.backup_verified = false;
    d = installer_preflight(&req);
    assert(!d.proceed);
    assert(d.next_state == INSTALLER_STATE_ABORTED);
    assert(d.error == INSTALLER_ERR_BAD_TOKEN);
    assert(strcmp(installer_error_text(d.error),
                  "installer package not authorized") == 0);

    free(g_img);
    g_img = NULL;
}


/* ------------------------------------------------------------------ */
/* 14h  Progress hook + pure percentage helper                         */
/* ------------------------------------------------------------------ */

static u32 g_progress_calls;
static installer_state_t g_progress_state;
static u32 g_progress_done;
static u32 g_progress_total;
static u8 g_progress_percent;

static void progress_cb(void *ctx, const installer_progress_t *p)
{
    (void)ctx;
    if (p == NULL) {
        return;
    }
    g_progress_calls++;
    g_progress_state = p->state;
    g_progress_done = p->done;
    g_progress_total = p->total;
    g_progress_percent = p->percent;
}

static void test_progress(void)
{
    static mem_dev_t dev;
    static mem_sink_t sink;
    installer_transfer_t xfer;
    u32 i;

    /* Pure percentage helper: clamps, never divides by zero, saturates. */
    assert(installer_percent(0u, 0u) == 0u);
    assert(installer_percent(5u, 0u) == 0u);
    assert(installer_percent(0u, 200u) == 0u);
    assert(installer_percent(50u, 200u) == 25u);
    assert(installer_percent(199u, 200u) == 99u);
    assert(installer_percent(200u, 200u) == 100u);
    assert(installer_percent(300u, 200u) == 100u);

    for (i = 0u; i < sizeof(dev.data); i++) {
        dev.data[i] = (u8)((i * 5u + 1u) & 0xFFu);
    }
    dev.sectors = TEST_DEV_SECTORS;

    /* With no hook installed, every report is a silent no-op. */
    installer_progress_state(INSTALLER_STATE_PREFLIGHT, "x");
    installer_progress_step(INSTALLER_STATE_BACKUP, 1u, 2u, "y");

    installer_set_progress_hook(progress_cb, NULL);

    /* Backup stream: one report per sector, monotonic, ends at 100%. */
    g_progress_calls = 0u;
    assert(installer_backup_stream(mem_dev_read, &dev, mem_sink_write, &sink,
                                   0u, 0x10u, &xfer) == INSTALLER_ERR_NONE);
    assert(g_progress_calls == 0x10u);
    assert(g_progress_state == INSTALLER_STATE_BACKUP);
    assert(g_progress_done == 0x10u);
    assert(g_progress_total == 0x10u);
    assert(g_progress_percent == 100u);

    /* Verify pass reports the distinct VERIFY_BACKUP stage. */
    assert(installer_backup_stream(mem_dev_read, &dev, mem_sink_write, &sink,
                                   0u, 4u, &xfer) == INSTALLER_ERR_NONE);
    g_progress_calls = 0u;
    assert(installer_backup_verify(mem_dev_read, &dev, xfer.digest, 0u, 4u) ==
           INSTALLER_ERR_NONE);
    assert(g_progress_calls == 4u);
    assert(g_progress_state == INSTALLER_STATE_VERIFY_BACKUP);
    assert(g_progress_percent == 100u);

    /* Flash engine: WRITE steps then a single VERIFY_WRITE transition. */
    {
        static mem_dev_t target;
        static mem_src_t src;
        u8 expect[SHA256_DIGEST_SIZE];

        memset(&target, 0, sizeof(target));
        target.sectors = TEST_DEV_SECTORS;
        src.data = dev.data;
        src.len = 4u * 512u;
        sha256(dev.data, 4u * 512u, expect);

        g_progress_calls = 0u;
        assert(installer_flash_region(mem_dev_read, &target,
                                      mem_dev_write, &target,
                                      mem_src_read, &src,
                                      0u, 4u, expect, &xfer) ==
               INSTALLER_ERR_NONE);
        assert(g_progress_calls == 5u);
        assert(g_progress_state == INSTALLER_STATE_VERIFY_WRITE);
        assert(g_progress_total == 0u);
        assert(g_progress_percent == 0u);
    }

    installer_set_progress_hook(NULL, NULL);
}

/* ------------------------------------------------------------------ */
/* 14g  Locked-build reporting                                         */
/* ------------------------------------------------------------------ */

static void test_locked_run(void)
{
    installer_report_t rep;

    installer_run(&rep);
    /* The host suite is built without ENABLE_NAND_WRITE_DANGER. */
    assert(!rep.write_enabled);
    assert(rep.state == INSTALLER_STATE_ABORTED);
    assert(rep.failed_state == INSTALLER_STATE_PREFLIGHT);
    assert(rep.error == INSTALLER_ERR_DISABLED);
    assert(rep.detail[0] != '\0');
    installer_run(NULL);   /* must not crash */
}

int main(int argc, char **argv)
{
    test_sha256_kat();
    test_hmac_kat();
    test_nand_layout();
    test_token();
    test_policy();
    test_state_machine();
    test_backup_flash();
    test_progress();
    test_fat_writer_reject();
    test_locked_run();

    if (argc > 1) {
        test_backup_verify_sd(argv[1]);
        test_fat_writer(argv[1]);
        test_no_token_preflight(argv[1]);
    }

    puts("test_installer: all assertions passed");
    return 0;
}
