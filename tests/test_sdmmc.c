/*
 * tests/test_sdmmc.c
 *
 * Mock-based unit tests for the ARM7 TMIO SD/MMC engine: the command
 * serializer, the FIFO block reader, and the bounded timeout / error handling.
 *
 * The driver is compiled with -DSDMMC_IO_HOOKS so every register access is
 * routed through the mock below instead of memory-mapped I/O. The mock models
 * the controller just enough to exercise the polling loops:
 *   - writing SD_CMD sets CMDRESPEND and, for a data read, queues a block and
 *     raises RXRDY;
 *   - reading the FIFO drains the block, then drops RXRDY and raises DATAEND;
 *   - writing the IRQ status acknowledges the selected flags.
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "arm7/sdmmc.h"

/* ------------------------------------------------------------------ */
/* Mock register file + controller state                               */
/* ------------------------------------------------------------------ */

/* Word-aligned so 16/32-bit accesses through (u8*)g_regs stay aligned. */
static u32 g_regs[0x80];

static u32  g_status;      /* 32-bit SD_IRQ_STATUS                       */
static u32  g_arg;         /* last value written to SD_CMD_PARAM         */
static u16  g_last_cmd;    /* last value written to SD_CMD               */
static u32  g_resp[4];     /* response words                             */
static u16  g_block[256];  /* the 512-byte block returned on a read      */
static u32  g_fifo_pos;
static u32  g_fifo_count;
static bool g_blocked;     /* simulate a card that never responds        */
static bool g_inject_err;  /* simulate a controller error                */

#define MOCK16(off) (*(u16 *)(void *)((u8 *)g_regs + (off)))
#define MOCK32(off) (*(u32 *)(void *)((u8 *)g_regs + (off)))

static void mock_reset(void)
{
    u32 i;

    memset(g_regs, 0, sizeof(g_regs));
    g_status = 0u;
    g_arg = 0u;
    g_last_cmd = 0u;
    g_fifo_pos = 0u;
    g_fifo_count = 0u;
    g_blocked = false;
    g_inject_err = false;
    memset(g_resp, 0, sizeof(g_resp));
    for (i = 0u; i < 256u; i++) {
        g_block[i] = (u16)(i * 3u + 1u);
    }
}

static u16 mock_fifo_pop(void)
{
    u16 w = g_block[g_fifo_pos];

    if (g_fifo_pos < g_fifo_count) {
        g_fifo_pos++;
    }
    if (g_fifo_pos >= g_fifo_count) {
        g_status &= ~(u32)TMIO_ST_RX_READY;
        g_status |= (u32)TMIO_ST_XFER_DONE;
    }
    return w;
}

static void mock_on_cmd(u16 val)
{
    g_last_cmd = val;

    if (g_blocked) {
        return;
    }
    if (g_inject_err) {
        g_status |= TMIO_ST_ERR_MASK;
        return;
    }

    if ((val & TMIO_CMD_DATA) != 0u) {
        g_fifo_pos = 0u;
        g_fifo_count = 256u;
        g_status |= (u32)TMIO_ST_RX_READY;
    }
    g_status |= (u32)TMIO_ST_CMD_DONE;
}

/* ---- IO hooks referenced by src/arm7/sdmmc.c ---- */

u16 sdmmc_io_read16(uintptr_t base, u32 off)
{
    (void)base;

    if (off == TMIO_REG_STATUS) {
        return (u16)(g_status & 0xFFFFu);
    }
    if (off == TMIO_REG_FIFO16) {
        return mock_fifo_pop();
    }
    if (off == TMIO_REG_WRPROTECT) {
        return 0u;
    }
    return MOCK16(off);
}

void sdmmc_io_write16(uintptr_t base, u32 off, u16 val)
{
    (void)base;

    if (off == TMIO_REG_CMD) {
        mock_on_cmd(val);
        return;
    }
    MOCK16(off) = val;
}

u32 sdmmc_io_read32(uintptr_t base, u32 off)
{
    (void)base;

    if (off == TMIO_REG_STATUS) {
        return g_status;
    }
    if (off >= TMIO_REG_RESP && off < TMIO_REG_RESP + 16u) {
        return g_resp[(off - TMIO_REG_RESP) / 4u];
    }
    return MOCK32(off);
}

void sdmmc_io_write32(uintptr_t base, u32 off, u32 val)
{
    (void)base;

    if (off == TMIO_REG_STATUS) {
        g_status &= val;   /* write 0 to acknowledge a flag */
        return;
    }
    if (off == TMIO_REG_ARG) {
        g_arg = val;
        return;
    }
    MOCK32(off) = val;
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_command_serializer(void)
{
    sdmmc_t c;
    u32 resp[4];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);

    g_resp[0] = 0x1AAu;
    assert(sdmmc_send_command(&c, 8u, 0x000001AAu, 1u, resp));

    /* Command index and argument reached the controller. */
    assert((g_last_cmd & TMIO_CMD_INDEX_MASK) == 8u);
    assert(g_arg == 0x000001AAu);
    assert(resp[0] == 0x1AAu);
}

static void test_block_read(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];
    u32 i;

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;        /* skip identification in this test */
    c.block_addressed = true;    /* LBA 0 -> argument 0              */
    c.timeout = 4096u;

    assert(sdmmc_read(&c, 0u, buf, sizeof(buf)));

    /* CMD17, with data, read direction. */
    assert((g_last_cmd & TMIO_CMD_INDEX_MASK) == 17u);
    assert((g_last_cmd & TMIO_CMD_DATA) != 0u);
    assert((g_last_cmd & TMIO_CMD_READ) != 0u);

    /* Block length was programmed and the argument is the block address. */
    assert(MOCK16(TMIO_REG_BLKLEN) == (u16)TMIO_BLOCK_BYTES);
    assert(MOCK16(TMIO_REG_BLKCNT) == 1u);
    assert(g_arg == 0u);

    /* The bytes match the mock block, little-endian per 16-bit word. */
    for (i = 0u; i < 256u; i++) {
        assert(buf[i * 2u] == (u8)(g_block[i] & 0xFFu));
        assert(buf[i * 2u + 1u] == (u8)(g_block[i] >> 8));
    }
}

static void test_block_addressed_argument(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;
    c.block_addressed = true;
    c.timeout = 4096u;

    assert(sdmmc_read(&c, 4u * TMIO_BLOCK_BYTES, buf, sizeof(buf)));
    assert(g_arg == 4u);   /* LBA 4, not byte 2048 */
}

static void test_byte_addressed_argument(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;
    c.block_addressed = false;
    c.timeout = 4096u;

    assert(sdmmc_read(&c, 4u * TMIO_BLOCK_BYTES, buf, sizeof(buf)));
    assert(g_arg == 4u * TMIO_BLOCK_BYTES);
}

static void test_timeout(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;
    c.timeout = 64u;
    g_blocked = true;   /* the card never raises CMDRESPEND */

    assert(!sdmmc_read(&c, 0u, buf, sizeof(buf)));
}

static void test_error(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;
    c.timeout = 64u;
    g_inject_err = true;   /* the controller reports a hard error */

    assert(!sdmmc_read(&c, 0u, buf, sizeof(buf)));
}

static void test_read_rejects_partial_block(void)
{
    sdmmc_t c;
    u8 buf[TMIO_BLOCK_BYTES];

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);
    c.initialized = true;
    c.timeout = 64u;

    assert(!sdmmc_read(&c, 0u, buf, 100u));   /* not a multiple of 512 */
}

static void test_get_status(void)
{
    sdmmc_t c;
    sdmmc_status_t st;

    mock_reset();
    sdmmc_init_instance(&c, 0u, false);

    /* SIGSTATE (bit5) = card inserted; WRPROTECT bit clear. */
    g_status = TMIO_ST_SIGSTATE;
    sdmmc_get_status(&c, &st);
    assert(st.present);
    assert(!st.error);
    assert(!st.initialized);

    g_status = TMIO_ST_SIGSTATE | TMIO_ST_ERR_MASK;
    sdmmc_get_status(&c, &st);
    assert(st.error);
}

int main(void)
{
    test_command_serializer();
    test_block_read();
    test_block_addressed_argument();
    test_byte_addressed_argument();
    test_timeout();
    test_error();
    test_read_rejects_partial_block();
    test_get_status();

    puts("test_sdmmc: all assertions passed");
    return 0;
}
