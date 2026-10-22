/*
 * src/arm7/sdmmc.c
 *
 * TWL TMIO SD/MMC controller driver. See include/arm7/sdmmc.h and
 * docs/hardware_notes.md §4 / §20.
 *
 * This is a clean-room translation of the reference TMIO engine:
 *   FUN_0001d930  generic command + data transfer (read direction)
 *   FUN_0001d958  generic command + data transfer (write direction)
 *   FUN_0001d750  read transfer into the staging buffer
 *   FUN_0001d6e4  block read framing (offset/len -> command + FIFO copy)
 *   FUN_0001d6b8  "wait until the card is ready" poll
 *   FUN_0001d154  controller reset/clock/option setup
 *
 * The reference drives the SDIO (wifi) controller at 0x04004A00; the SD/MMC
 * controller at 0x04004800 shares the same register model, so the same engine
 * is applied here with standard SD/MMC commands (CMD17 for a block read).
 *
 * Robustness: every hardware poll is bounded by `c->timeout` so an absent or
 * unresponsive card can never wedge the ARM7 (unlike the reference's unbounded
 * `do { } while` loops).
 *
 * SAFETY (AGENTS.md §5): the eMMC path is read-only. The write helper is
 * compiled out unless ENABLE_NAND_WRITE_DANGER is defined.
 */
#include "arm7/sdmmc.h"
#include "common/dsi_hw.h"

/* ------------------------------------------------------------------ */
/* Register access                                                     */
/* ------------------------------------------------------------------ */
/*
 * On the target the registers are memory-mapped; on the host test harness the
 * same accesses are routed to a mock register file (SDMMC_IO_HOOKS), so the
 * FIFO reader and command serializer can be unit-tested without hardware.
 */
#ifdef SDMMC_IO_HOOKS
u16  sdmmc_io_read16(uintptr_t base, u32 off);
void sdmmc_io_write16(uintptr_t base, u32 off, u16 val);
u32  sdmmc_io_read32(uintptr_t base, u32 off);
void sdmmc_io_write32(uintptr_t base, u32 off, u32 val);

#define SD_R16(b, o)    sdmmc_io_read16((b), (o))
#define SD_W16(b, o, v) sdmmc_io_write16((b), (o), (v))
#define SD_R32(b, o)    sdmmc_io_read32((b), (o))
#define SD_W32(b, o, v) sdmmc_io_write32((b), (o), (v))
#else
#define SD_R16(b, o)    REG16((b) + (o))
#define SD_W16(b, o, v) (REG16((b) + (o)) = (v))
#define SD_R32(b, o)    REG32((b) + (o))
#define SD_W32(b, o, v) (REG32((b) + (o)) = (v))
#endif

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

u32 sdmmc_status(const sdmmc_t *c)
{
    return SD_R32(c->base, TMIO_REG_STATUS);
}

bool sdmmc_has_error(const sdmmc_t *c)
{
    return tmio_status_error(sdmmc_status(c));
}

/* Build an SD_CMD register word (see the field layout in sdmmc.h). */
u16 tmio_cmd_word(u32 index, u32 type, u32 resp,
                  bool data, bool read, bool multi)
{
    u16 v = (u16)(index & TMIO_CMD_INDEX_MASK);

    v |= (u16)((type & 0x3u) << TMIO_CMD_TYPE_SHIFT);
    v |= (u16)((resp & 0x7u) << TMIO_CMD_RESP_SHIFT);
    if (data) {
        v |= (u16)TMIO_CMD_DATA;
    }
    if (read) {
        v |= (u16)TMIO_CMD_READ;
    }
    if (multi) {
        v |= (u16)TMIO_CMD_MULTI;
    }
    return v;
}

/*
 * Bounded status poll: wait until (status & mask) == mask, aborting on any
 * hard error. Returns false on error or when the poll budget is exhausted.
 */
static bool tmio_wait_bits(const sdmmc_t *c, u32 mask, u32 *out_status)
{
    u32 i;

    for (i = 0u; i < c->timeout; i++) {
        u32 st = SD_R32(c->base, TMIO_REG_STATUS);

        if ((st & TMIO_ST_ERR_MASK) != 0u) {
            if (out_status != NULL) {
                *out_status = st;
            }
            return false;
        }
        if ((st & mask) == mask) {
            if (out_status != NULL) {
                *out_status = st;
            }
            return true;
        }
    }

    if (out_status != NULL) {
        *out_status = SD_R32(c->base, TMIO_REG_STATUS);
    }
    return false;
}

/* Crude busy-wait used between card-identification retries. */
static void tmio_delay(u32 loops)
{
    volatile u32 i;

    for (i = 0u; i < loops; i++) {
        /* spin */
    }
}

static void tmio_read_response(const sdmmc_t *c, u32 words, u32 *resp)
{
    u32 i;

    if (resp == NULL) {
        return;
    }
    if (words > 4u) {
        words = 4u;
    }
    for (i = 0u; i < words; i++) {
        resp[i] = SD_R32(c->base, TMIO_REG_RESP + i * 4u);
    }
}

/*
 * Push one command: argument, acknowledge stale IRQs, then the command word
 * itself. When `wait_done` is set, poll CMDRESPEND with a bounded timeout.
 */
static bool tmio_issue(const sdmmc_t *c, u16 cmd_word, u32 arg, bool wait_done)
{
    SD_W32(c->base, TMIO_REG_ARG, arg);
    SD_W32(c->base, TMIO_REG_STATUS,
           SD_R32(c->base, TMIO_REG_STATUS) & TMIO_ST_ACK_MASK);
    SD_W16(c->base, TMIO_REG_CMD, cmd_word);

    if (!wait_done) {
        return true;
    }
    return tmio_wait_bits(c, TMIO_ST_CMD_DONE, NULL);
}

/* Clock `len` bytes out of the RX FIFO, waiting for RXRDY before each word. */
static bool tmio_rx(const sdmmc_t *c, u8 *dst, u32 len)
{
    u32 words = len / 2u;
    u32 i;

    for (i = 0u; i < words; i++) {
        u16 w;

        if (!tmio_wait_bits(c, TMIO_ST_RX_READY, NULL)) {
            return false;
        }
        w = SD_R16(c->base, TMIO_REG_FIFO16);
        dst[i * 2u] = (u8)w;
        dst[i * 2u + 1u] = (u8)(w >> 8);
    }

    if ((len & 1u) != 0u) {
        if (!tmio_wait_bits(c, TMIO_ST_RX_READY, NULL)) {
            return false;
        }
        dst[len - 1u] = (u8)(SD_R16(c->base, TMIO_REG_FIFO16) & 0xFFu);
    }

    return tmio_wait_bits(c, TMIO_ST_XFER_DONE, NULL);
}

/* ------------------------------------------------------------------ */
/* Public command / read API                                           */
/* ------------------------------------------------------------------ */

bool sdmmc_send_command(const sdmmc_t *c, u32 cmd_index, u32 arg,
                        u32 resp_words, u32 *resp)
{
    u16 cmd_word;

    if (c == NULL) {
        return false;
    }

    cmd_word = tmio_cmd_word(cmd_index, TMIO_CMD_TYPE_CMD, TMIO_CMD_RESP_AUTO,
                             false, false, false);
    if (!tmio_issue(c, cmd_word, arg, resp_words != 0u)) {
        return false;
    }
    tmio_read_response(c, resp_words, resp);
    return true;
}

/* Single 512-byte READ_SINGLE_BLOCK (CMD17). */
static bool tmio_read_block(sdmmc_t *c, u32 offset, u8 *dst)
{
    u32 arg = c->block_addressed ? (offset / TMIO_BLOCK_BYTES) : offset;
    u16 cmd_word = tmio_cmd_word(17u, TMIO_CMD_TYPE_CMD, TMIO_CMD_RESP_AUTO,
                                 true, true, false);
    u32 resp[1];

    SD_W16(c->base, TMIO_REG_DATACTL, 0x0000u);   /* DATA16 mode           */
    SD_W16(c->base, TMIO_REG_DATA32IRQ, 0x0000u);
    SD_W16(c->base, TMIO_REG_BLKLEN, (u16)TMIO_BLOCK_BYTES);
    SD_W16(c->base, TMIO_REG_BLKCNT, 1u);
    SD_W16(c->base, TMIO_REG_STOP, 0u);           /* no auto CMD12         */

    if (!tmio_issue(c, cmd_word, arg, true)) {
        return false;
    }
    tmio_read_response(c, 1u, resp);

    return tmio_rx(c, dst, TMIO_BLOCK_BYTES);
}

bool sdmmc_read(sdmmc_t *c, u32 offset, void *dst, u32 len)
{
    u8 *p = (u8 *)dst;

    if (c == NULL || dst == NULL || len == 0u) {
        return false;
    }
    if ((len % TMIO_BLOCK_BYTES) != 0u) {
        return false;   /* only whole 512-byte blocks are supported */
    }

    /*
     * The card is expected to be in the transfer state already: the reference
     * read path (FUN_0001d6e4) issues its data command without re-identifying
     * the card. Callers that need a full cold start can run sdmmc_card_init()
     * first (it also resolves the byte/block addressing mode).
     */
    while (len != 0u) {
        if (!tmio_read_block(c, offset, p)) {
            return false;
        }
        offset += TMIO_BLOCK_BYTES;
        p += TMIO_BLOCK_BYTES;
        len -= TMIO_BLOCK_BYTES;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Controller + card bring-up                                          */
/* ------------------------------------------------------------------ */

void sdmmc_init_instance(sdmmc_t *c, uintptr_t base, bool is_emmc)
{
    if (c == NULL) {
        return;
    }
    c->base = base;
    c->is_emmc = is_emmc;
    c->initialized = false;
    c->card_ready = false;
    c->block_addressed = false;
    c->rca = 0u;
    c->timeout = TMIO_DEFAULT_TIMEOUT;
}

/*
 * Bring the DSi SD/MMC block out of reset: SCFG_EXT7.bit18 grants the ARM7
 * access to 0x04004800-0x040049FF and SCFG_CLK7.bit0 runs the block clock.
 * Both are required before any TMIO register is read or written. A no-op on a
 * legacy DS and on a DSi running an NDS title (see docs/hardware_notes.md §24).
 */
void sdmmc_enable_clock(void)
{
#ifdef SDMMC_IO_HOOKS
    /* Host mock: there is no SCFG page, nothing to enable. */
#else
    u16 a9rom = (u16)(REG16(SCFG_A9ROM) & SCFG_A9ROM_MODE_MASK);
    u32 ext;

    /*
     * Only a DSi in DSi mode (A9ROM == 0x01) owns the TMIO block. 0x03 is a
     * DSi running an NDS title (DS compatibility) and 0x00 a legacy DS / hidden
     * page; in both cases the SCFG page must not be reprogrammed.
     */
    if (a9rom != SCFG_A9ROM_DSI_MODE) {
        return;
    }

    /* Grant access to the SD/MMC register window (keep SCFG/MBK unlocked). */
    ext = REG32(SCFG_EXT);
    ext |= SCFG_EXT7_SDMMC_ACCESS | SCFG_EXT_SCFG_ACCESS;
    REG32(SCFG_EXT) = ext;

    /* Run the SD/MMC block clock (mirrors SCFG_EXT7.bit18). */
    REG16(SCFG_CLK) = (u16)(REG16(SCFG_CLK) | SCFG_CLK7_SDMMC);
#endif
}

void sdmmc_init_controller(sdmmc_t *c)
{
    if (c == NULL) {
        return;
    }

    /* Enable the DSi block clock/power before touching any TMIO register. */
    sdmmc_enable_clock();

    /* Slot select: bit0 chooses the SD card (0) or the onboard eMMC (1). */
    SD_W16(c->base, TMIO_REG_PORTSEL,
           (u16)(TMIO_PORTSEL_SD | (c->is_emmc ? 1u : 0u)));

    /* Assert, then release, the soft reset. */
    SD_W16(c->base, TMIO_REG_SOFTRESET, 0x0000u);
    SD_W16(c->base, TMIO_REG_SOFTRESET, 0x0001u);

    /* Card option + slow identification clock. */
    SD_W16(c->base, TMIO_REG_OPTION, TMIO_OPTION_DEFAULT);
    SD_W16(c->base, TMIO_REG_CLK, (u16)(TMIO_CLK_ENABLE | TMIO_CLK_DIV_INIT));

    /* Polled driver: mask every interrupt source. */
    SD_W32(c->base, TMIO_REG_IRQMASK, 0x8B7F031Du);

    /* Acknowledge any stale flags. */
    SD_W32(c->base, TMIO_REG_STATUS,
           SD_R32(c->base, TMIO_REG_STATUS) & TMIO_ST_ACK_MASK);
}

/*
 * Card identification. Best-effort: a command that does not complete (e.g.
 * CMD8 on an MMC that does not implement it, or under an emulator) is skipped
 * rather than treated as fatal, so the caller can still attempt a direct read.
 */
bool sdmmc_card_init(sdmmc_t *c)
{
    u32 resp[4];
    u32 ocr = 0u;
    u32 i;
    bool ready = false;

    if (c == NULL) {
        return false;
    }

    c->card_ready = false;
    c->initialized = true;
    c->timeout = TMIO_INIT_TIMEOUT;

    /* Slow clock for identification. */
    SD_W16(c->base, TMIO_REG_CLK, (u16)(TMIO_CLK_ENABLE | TMIO_CLK_DIV_INIT));

    /* CMD0: GO_IDLE_STATE (no response). */
    (void)tmio_issue(c, tmio_cmd_word(0u, TMIO_CMD_TYPE_CMD, TMIO_CMD_RESP_NONE,
                                      false, false, false), 0u, false);

    /* CMD8: SEND_IF_COND (best-effort). */
    (void)sdmmc_send_command(c, 8u, 0x000001AAu, 1u, resp);

    /*
     * ACMD41 (SD) or CMD1 (eMMC) until the card reports ready. The card may
     * need up to ~1 s, so space the retries with a busy-wait.
     */
    for (i = 0u; i < 100u; i++) {
        bool ok;

        if (c->is_emmc) {
            ok = sdmmc_send_command(c, 1u, 0x40FF8000u, 1u, resp);
        } else {
            (void)sdmmc_send_command(c, 55u, 0u, 1u, resp);   /* APP_CMD   */
            ok = sdmmc_send_command(c, 41u, 0x40FF8000u, 1u, resp);
        }

        if (ok) {
            ocr = resp[0];
            if (tmio_ocr_ready(ocr)) {
                ready = true;
                break;
            }
        }
        tmio_delay(0x8000u);
    }

    /* OCR bit30 (CCS) selects block vs byte addressing for data commands. */
    c->block_addressed = tmio_ocr_block_addressed(ocr);

    /* CMD2: ALL_SEND_CID (R2). */
    (void)sdmmc_send_command(c, 2u, 0u, 4u, resp);

    /* CMD3: SEND_RELATIVE_ADDR (R6) -> RCA. */
    if (sdmmc_send_command(c, 3u, 0u, 1u, resp)) {
        c->rca = resp[0] >> 16;
    }

    /* CMD9: SEND_CSD (R2). */
    (void)sdmmc_send_command(c, 9u, c->rca << 16, 4u, resp);

    /* CMD7: SELECT_CARD. */
    (void)sdmmc_send_command(c, 7u, c->rca << 16, 1u, resp);

    /* CMD16: SET_BLOCKLEN = 512 (ignored by block-addressed cards). */
    (void)sdmmc_send_command(c, 16u, TMIO_BLOCK_BYTES, 1u, resp);

    /* Switch to the fast transfer clock. */
    SD_W16(c->base, TMIO_REG_CLK, (u16)(TMIO_CLK_ENABLE | TMIO_CLK_DIV_FAST));

    c->timeout = TMIO_DEFAULT_TIMEOUT;
    c->card_ready = ready;
    return ready;
}

bool sdmmc_open(sdmmc_t *c)
{
    if (c == NULL) {
        return false;
    }
    sdmmc_init_controller(c);
    c->initialized = true;
    return true;
}

void sdmmc_get_status(const sdmmc_t *c, sdmmc_status_t *out)
{
    u32 status;
    u16 wp;

    if (c == NULL || out == NULL) {
        return;
    }

    status = SD_R32(c->base, TMIO_REG_STATUS);
    wp = SD_R16(c->base, TMIO_REG_WRPROTECT);

    out->raw_status = status;
    out->error = tmio_status_error(status);
    out->present = tmio_status_card_present(status);
    out->initialized = c->initialized;
    out->write_protected = (wp & 0x0001u) != 0u;
}

/* ------------------------------------------------------------------ */
/* eMMC write path (gated, AGENTS.md §5)                               */
/* ------------------------------------------------------------------ */
#ifdef ENABLE_NAND_WRITE_DANGER

/* Clock `len` bytes into the TX FIFO, waiting for TXRQ before each word. */
static bool tmio_tx(const sdmmc_t *c, const u8 *src, u32 len)
{
    u32 words = len / 2u;
    u32 i;

    for (i = 0u; i < words; i++) {
        if (!tmio_wait_bits(c, TMIO_ST_TX_READY, NULL)) {
            return false;
        }
        SD_W16(c->base, TMIO_REG_FIFO16,
               (u16)((u16)src[i * 2u] | ((u16)src[i * 2u + 1u] << 8)));
    }

    return tmio_wait_bits(c, TMIO_ST_XFER_DONE, NULL);
}

static bool tmio_write_block(sdmmc_t *c, u32 offset, const u8 *src)
{
    u32 arg = c->block_addressed ? (offset / TMIO_BLOCK_BYTES) : offset;
    u16 cmd_word = tmio_cmd_word(24u, TMIO_CMD_TYPE_CMD, TMIO_CMD_RESP_AUTO,
                                 true, false, false);
    u32 resp[1];

    SD_W16(c->base, TMIO_REG_DATACTL, 0x0000u);
    SD_W16(c->base, TMIO_REG_DATA32IRQ, 0x0000u);
    SD_W16(c->base, TMIO_REG_BLKLEN, (u16)TMIO_BLOCK_BYTES);
    SD_W16(c->base, TMIO_REG_BLKCNT, 1u);
    SD_W16(c->base, TMIO_REG_STOP, 0u);

    if (!tmio_issue(c, cmd_word, arg, true)) {
        return false;
    }
    tmio_read_response(c, 1u, resp);

    return tmio_tx(c, src, TMIO_BLOCK_BYTES);
}

bool sdmmc_write(const sdmmc_t *c, u32 offset, const void *src, u32 len)
{
    const u8 *p = (const u8 *)src;
    sdmmc_t *mutable_c = (sdmmc_t *)c;   /* init state lives here */

    if (c == NULL || src == NULL || len == 0u) {
        return false;
    }
    if ((len % TMIO_BLOCK_BYTES) != 0u) {
        return false;
    }
    if (!c->initialized) {
        (void)sdmmc_card_init(mutable_c);
    }

    while (len != 0u) {
        if (!tmio_write_block(mutable_c, offset, p)) {
            return false;
        }
        offset += TMIO_BLOCK_BYTES;
        p += TMIO_BLOCK_BYTES;
        len -= TMIO_BLOCK_BYTES;
    }
    return true;
}

#endif /* ENABLE_NAND_WRITE_DANGER */
