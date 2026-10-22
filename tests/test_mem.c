/*
 * tests/test_mem.c
 *
 * Host-side unit test for the pure (hardware-independent) shared primitives:
 * dsi_mem{set,cpy,cmp}, dsi_strlen, little-endian accessors, and IPC packing.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "common/mem.h"
#include "common/ipc_protocol.h"

static void test_memset(void)
{
    unsigned char buf[37];

    memset(buf, 0xAA, sizeof(buf));
    dsi_memset(buf, 0x55, sizeof(buf));
    for (unsigned i = 0; i < sizeof(buf); i++) {
        assert(buf[i] == 0x55);
    }

    /* Odd length + offset: exercises the scalar head/tail paths. */
    dsi_memset(buf + 3, 0x00, 17);
    for (unsigned i = 3; i < 20; i++) {
        assert(buf[i] == 0x00);
    }
    assert(buf[2] == 0x55 && buf[20] == 0x55);
}

static void test_memcpy(void)
{
    unsigned char src[40];
    unsigned char dst[40];

    for (unsigned i = 0; i < sizeof(src); i++) {
        src[i] = (unsigned char)i;
    }
    dsi_memcpy(dst, src, sizeof(src));
    assert(memcmp(dst, src, sizeof(src)) == 0);

    /* Misaligned source/destination. */
    dsi_memcpy(dst + 3, src + 1, 20);
    assert(memcmp(dst + 3, src + 1, 20) == 0);
}

static void test_memcmp_strlen(void)
{
    assert(dsi_memcmp("abc", "abc", 3) == 0);
    assert(dsi_memcmp("abc", "abd", 3) < 0);
    assert(dsi_strlen("hello") == 5);
    assert(dsi_strlen("") == 0);
}

static void test_le(void)
{
    unsigned char buf[8];
    dsi_write_le32(buf, 0x11223344u);

    assert(buf[0] == 0x44 && buf[1] == 0x33 && buf[2] == 0x22 && buf[3] == 0x11);
    assert(dsi_read_le32(buf) == 0x11223344u);
    assert(dsi_read_le16(buf) == 0x3344u);
    assert(dsi_read_u8(buf) == 0x44u);
}

static void test_ipc(void)
{
    u32 w = ipc_pack(IPC_CH_FS, IPC_CMD_FS_OPEN);
    assert(ipc_channel(w) == IPC_CH_FS);
    assert(ipc_payload(w) == IPC_CMD_FS_OPEN);
}

static void test_ipc_reply(void)
{
    u32 h = ipc_reply_header(IPC_CMD_GET_RTC, IPC_REPLY_RTC_WORDS,
                             IPC_RTC_STATUS_VALID);
    u32 t;

    assert((h & 0xFFFFu) == IPC_CMD_GET_RTC);
    assert(ipc_reply_words(h) == IPC_REPLY_RTC_WORDS);
    assert(ipc_reply_status(h) == IPC_RTC_STATUS_VALID);

    t = ipc_pack_triple(0x12u, 0x34u, 0x56u);
    assert(ipc_unpack_triple_hi(t) == 0x12u);
    assert(ipc_unpack_triple_mid(t) == 0x34u);
    assert(ipc_unpack_triple_lo(t) == 0x56u);
}

static void test_ipc_request(void)
{
    u32 h = ipc_request_header(IPC_CMD_READ_SECTOR,
                               IPC_REQ_READ_SECTOR_WORDS);

    assert((h & 0xFFFFu) == IPC_CMD_READ_SECTOR);
    assert(ipc_request_words(h) == IPC_REQ_READ_SECTOR_WORDS);

    /* A zero-argument header is byte-identical to a legacy raw opcode. */
    assert(ipc_request_header(IPC_CMD_GET_RTC, 0u) == IPC_CMD_GET_RTC);
    assert(ipc_request_words(IPC_CMD_GET_RTC) == 0u);

    /* The channel nibble is added on top and does not disturb the count. */
    h = ipc_pack(IPC_CH_SYSTEM,
                 ipc_request_header(IPC_CMD_READ_FIRMWARE,
                                    IPC_REQ_READ_FIRMWARE_WORDS));
    assert(ipc_channel(h) == IPC_CH_SYSTEM);
    assert((ipc_payload(h) & 0xFFFFu) == IPC_CMD_READ_FIRMWARE);
    assert(ipc_request_words(ipc_payload(h)) == IPC_REQ_READ_FIRMWARE_WORDS);
}

static void test_shared_ram(void)
{
    assert(ipc_addr_is_shared_ram(IPC_SHARED_RAM_BASE, 4u));
    assert(ipc_addr_is_shared_ram(IPC_SHARED_RAM_END - 4u, 4u));
    assert(!ipc_addr_is_shared_ram(IPC_SHARED_RAM_END - 3u, 4u));
    assert(!ipc_addr_is_shared_ram(IPC_SHARED_RAM_BASE - 1u, 4u));
    assert(!ipc_addr_is_shared_ram(0x0B000000u, 4u));   /* DTCM, ARM9-only */
    assert(!ipc_addr_is_shared_ram(IPC_SHARED_RAM_BASE, 0u));
    assert(!ipc_addr_is_shared_ram(IPC_SHARED_RAM_BASE, 0xFFFFFFFFu));
}

int main(void)
{
    test_memset();
    test_memcpy();
    test_memcmp_strlen();
    test_le();
    test_ipc();
    test_ipc_reply();
    test_ipc_request();
    test_shared_ram();

    puts("host-test: all assertions passed");
    return 0;
}
