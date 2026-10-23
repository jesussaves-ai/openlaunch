/*
 * include/arm7/sdmmc.h
 *
 * TWL TMIO SD/MMC controller driver (external SD card + onboard eMMC).
 *
 * The register block is the Toshiba/DSi "SD/MMC" controller at 0x04004800;
 * the SDIO (wifi) controller at 0x04004A00 shares the exact same register
 * layout. The reference routines FUN_0001d930 / FUN_0001d958 / FUN_0001d750 /
 * FUN_0001d6e4 drive the SDIO controller, but their FIFO/status/DMA mechanics
 * are identical to the SD/MMC controller we target here.
 *
 * Register offsets and bit meanings are taken from the dsibrew
 * "SD/MMC/SDIO Registers" page and cross-checked against the reference
 * disassembly (see docs/hardware_notes.md §4 and §20).
 *
 * SAFETY (the project conventions): the internal eMMC is READ-ONLY by default. No code in
 * this module issues a write/erase command unless ENABLE_NAND_WRITE_DANGER is
 * defined at build time.
 */
#ifndef DSI_ARM7_SDMMC_H
#define DSI_ARM7_SDMMC_H

#include "../common/types.h"

/* ---- Controller base addresses (the project conventions) ---- */
#define TMIO_SD_BASE    0x04004800u   /* SD/MMC: SD card + onboard eMMC    */
#define TMIO_SDIO_BASE  0x04004A00u   /* SDIO (wifi): same register model  */

/* SD_CARD_PORT_SELECT: bit0 selects the slot (0=SD card, 1=eMMC); bits 9-8
 * read back as 2 on the SD/MMC controller. */
#define TMIO_PORTSEL_SD    0x0200u
#define TMIO_PORTSEL_EMMC  0x0201u

/* ---- Register offsets ---- */
#define TMIO_REG_CMD        0x000u   /* 16-bit command + response/data type  */
#define TMIO_REG_PORTSEL    0x002u   /* 16-bit slot select                   */
#define TMIO_REG_ARG        0x004u   /* 32-bit command argument              */
#define TMIO_REG_STOP       0x008u   /* 16-bit stop/auto-CMD12 control       */
#define TMIO_REG_BLKCNT     0x00Au   /* 16-bit DATA16 block count            */
#define TMIO_REG_RESP       0x00Cu   /* 128-bit response (8 halfwords)       */
#define TMIO_REG_STATUS     0x01Cu   /* 32-bit IRQ status (0x1C low,0x1E hi) */
#define TMIO_REG_IRQMASK    0x020u   /* 32-bit IRQ mask (1=disable)          */
#define TMIO_REG_CLK        0x024u   /* 16-bit card clock control            */
#define TMIO_REG_BLKLEN     0x026u   /* 16-bit DATA16 block length           */
#define TMIO_REG_OPTION     0x028u   /* 16-bit card option setup             */
#define TMIO_REG_FIFO16     0x030u   /* 16-bit data port                     */
#define TMIO_REG_DATACTL    0x0D8u   /* 16-bit data control (16/32-bit mode) */
#define TMIO_REG_SOFTRESET  0x0E0u   /* 16-bit soft reset                    */
#define TMIO_REG_WRPROTECT  0x0F6u   /* write-protect probe                  */
#define TMIO_REG_DATA32IRQ  0x100u   /* 16-bit DATA32 IRQ control            */
#define TMIO_REG_BLKLEN32   0x104u   /* 16-bit DATA32 block length           */
#define TMIO_REG_BLKCNT32   0x108u   /* 16-bit DATA32 block count            */
#define TMIO_REG_FIFO32     0x10Cu   /* 32-bit data port                     */

/* ---- SD_CMD field layout (dsibrew) ----
 *
 *   bits 5-0   command index
 *   bits 7-6   command type (0=CMD, 1=ACMD)
 *   bits 10-8  response type (0=Auto, 3=None, 4=48-bit, 5=48-bit+busy,
 *              6=136-bit, 7=48-bit OCR)
 *   bit  11    data transfer present
 *   bit  12    data direction (1=read)
 *   bit  13    multi-block
 */
#define TMIO_CMD_INDEX_MASK  0x003Fu
#define TMIO_CMD_TYPE_SHIFT  6u
#define TMIO_CMD_TYPE_CMD    0x0u
#define TMIO_CMD_TYPE_ACMD   0x1u
#define TMIO_CMD_RESP_SHIFT  8u
#define TMIO_CMD_RESP_AUTO   0x0u
#define TMIO_CMD_RESP_NONE   0x3u
#define TMIO_CMD_RESP_48     0x4u
#define TMIO_CMD_RESP_48B    0x5u
#define TMIO_CMD_RESP_136    0x6u
#define TMIO_CMD_RESP_OCR    0x7u
#define TMIO_CMD_DATA        BIT(11)
#define TMIO_CMD_READ        BIT(12)
#define TMIO_CMD_MULTI       BIT(13)

/* ---- SD_IRQ_STATUS bits ---- */
#define TMIO_ST_CMD_DONE     BIT(0)    /* CMDRESPEND (response end)         */
#define TMIO_ST_XFER_DONE    BIT(2)    /* DATAEND                           */
#define TMIO_ST_CARD_REMOVE  BIT(3)
#define TMIO_ST_CARD_INSERT  BIT(4)
#define TMIO_ST_SIGSTATE     BIT(5)    /* 1 = card inserted                 */
#define TMIO_ST_WRPROTECT    BIT(7)    /* 1 = unlocked                      */
#define TMIO_ST_ERR_MASK     0x807F0000u /* CMD/CRC/END/timeout + ILA      */
#define TMIO_ST_RX_READY     BIT(24)   /* RX FIFO not empty                 */
#define TMIO_ST_TX_READY     BIT(25)   /* TX FIFO wants data                */
#define TMIO_ST_ILLEGAL      BIT(31)
#define TMIO_ST_ACK_MASK     0x7C80FFFAu /* write back to acknowledge IRQs  */

/* ---- SD_CLK_CTL ---- */
#define TMIO_CLK_ENABLE      BIT(8)    /* 1 = output SDCLK                  */
#define TMIO_CLK_DIV_INIT    0x80u     /* HCLK/256 during identification    */
#define TMIO_CLK_DIV_FAST    0x01u     /* HCLK/4 after identification       */

/* ---- SD_CARD_OPTION default (dsibrew) ---- */
#define TMIO_OPTION_DEFAULT  0x40EEu

/* ---- Transfer limits / defaults ---- */
#define TMIO_BLOCK_BYTES       512u
#define TMIO_DEFAULT_TIMEOUT   0x00100000u
#define TMIO_INIT_TIMEOUT      0x00080000u

typedef struct {
    uintptr_t base;          /* TMIO_SD_BASE (mock pointer on host)         */
    bool  is_emmc;           /* onboard eMMC => read-only policy applies    */
    bool  initialized;       /* controller programmed / card init attempted */
    bool  card_ready;        /* card reached the transfer state (CMD7 ok)   */
    bool  block_addressed;   /* true => command argument is a block (LBA)   */
    u32   rca;               /* relative card address from CMD3             */
    u32   timeout;           /* poll iterations per wait                    */
} sdmmc_t;

/*
 * Read-only snapshot of the controller / card state. All fields come from
 * reads only (the project conventions).
 */
typedef struct {
    bool present;          /* slot reports a card (SIGSTATE)              */
    bool initialized;      /* sdmmc_card_init() has been attempted        */
    bool write_protected;  /* write-protect status bit                    */
    bool error;            /* any status error bit set                    */
    u32  raw_status;       /* raw TMIO_REG_STATUS                         */
} sdmmc_status_t;

/* ------------------------------------------------------------------ */
/* Pure decoders (host-testable; no register access)                   */
/* ------------------------------------------------------------------ */

/*
 * Build an SD_CMD register word from its fields (see the field layout above).
 * Exposed so the command serializer can be unit-tested without hardware.
 */
u16 tmio_cmd_word(u32 index, u32 type, u32 resp,
                  bool data, bool read, bool multi);

/* True when any hard-error bit is set in a raw SD_IRQ_STATUS word. */
static inline bool tmio_status_error(u32 status)
{
    return (status & TMIO_ST_ERR_MASK) != 0u;
}

/*
 * True when the slot reports a card. 0xFFFFFFFF is the "controller absent /
 * open bus" value a legacy DS returns, so it is explicitly not "present".
 */
static inline bool tmio_status_card_present(u32 status)
{
    return (status != 0xFFFFFFFFu) && ((status & TMIO_ST_SIGSTATE) != 0u);
}

/* True when CMDRESPEND (command response end) is set. */
static inline bool tmio_status_cmd_done(u32 status)
{
    return (status & TMIO_ST_CMD_DONE) != 0u;
}

/* True when DATAEND (last data block end) is set. */
static inline bool tmio_status_xfer_done(u32 status)
{
    return (status & TMIO_ST_XFER_DONE) != 0u;
}

/* OCR bit 31: card power-up / ACMD41 busy-complete. */
static inline bool tmio_ocr_ready(u32 ocr)
{
    return (ocr & 0x80000000u) != 0u;
}

/* OCR bit 30 (CCS): 1 => SDHC/SDXC (block addressing), 0 => SDSC (byte). */
static inline bool tmio_ocr_block_addressed(u32 ocr)
{
    return (ocr & (1u << 30)) != 0u;
}

/* Read the raw 32-bit IRQ status word. */
u32 sdmmc_status(const sdmmc_t *c);

/* True when the controller reports a hard error. */
bool sdmmc_has_error(const sdmmc_t *c);

/* Populate `c` with defaults for the given controller base. */
void sdmmc_init_instance(sdmmc_t *c, uintptr_t base, bool is_emmc);

/*
 * Enable the DSi SD/MMC controller block on the ARM7: set
 * SCFG_EXT7.bit18 (register-window access) and SCFG_CLK7.bit0 (block clock).
 * Self-gating: a no-op on a legacy DS (no SCFG page) or when the DSi is
 * running an NDS title. Read-only with respect to the card (the project conventions).
 *
 * Compiles to an empty stub under the host mock (SDMMC_IO_HOOKS).
 */
void sdmmc_enable_clock(void);

/*
 * Program the controller: enable the block clock/power, soft reset, card
 * option, clock, slot select.
 *
 * @ref FUN_0001d154  (address 0x01D154) — the reference reset/clock sequence
 * (against the SDIO controller; identical layout).
 */
void sdmmc_init_controller(sdmmc_t *c);

/*
 * Best-effort SD/MMC card identification: CMD0, CMD8, ACMD41/CMD1, CMD2,
 * CMD3, CMD9, CMD7, CMD16. Returns true if the card reached the transfer
 * state. As a side effect it sets `block_addressed` from the OCR CCS bit and
 * `card_ready` on success, so it should be run before a read on a
 * high-capacity card. Every command is bounded by `c->timeout`, so an absent
 * or unresponsive card can never wedge the ARM7.
 *
 * It is NOT called automatically by sdmmc_read(); the ARM7 IPC layer
 * (src/arm7/ipc.c) runs it once per slot before the first read.
 */
bool sdmmc_card_init(sdmmc_t *c);

/*
 * Program the controller and mark the instance ready for transfers. Does not
 * run card identification (see sdmmc_card_init()).
 */
bool sdmmc_open(sdmmc_t *c);

/*
 * Issue one command. `resp_words` is 0 (no response) or the number of 32-bit
 * words to capture (1 for R1/R3/R6/R7, 4 for R2 CID/CSD). Returns true when
 * the command completed without a status error.
 */
bool sdmmc_send_command(const sdmmc_t *c, u32 cmd_index, u32 arg,
                        u32 resp_words, u32 *resp);

/*
 * Read `len` bytes from the card at byte `offset` into `dst`. `len` must be a
 * non-zero multiple of 512; each 512-byte block is fetched with CMD17
 * (READ_SINGLE_BLOCK). Returns true on success.
 */
bool sdmmc_read(sdmmc_t *c, u32 offset, void *dst, u32 len);

/*
 * Probe the controller/card state using reads only. Never issues a command
 * and never writes to the card.
 */
void sdmmc_get_status(const sdmmc_t *c, sdmmc_status_t *out);

/*
 * eMMC write. Compiled only when ENABLE_NAND_WRITE_DANGER is defined.
 *
 * @ref FUN_0001d760 (write path).
 */
#ifdef ENABLE_NAND_WRITE_DANGER
bool sdmmc_write(const sdmmc_t *c, u32 offset, const void *src, u32 len);
#endif

#endif /* DSI_ARM7_SDMMC_H */
