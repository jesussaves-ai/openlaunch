/*
 * src/arm7/ipc.c
 *
 * ARM7 IPC FIFO bridge. See include/arm7/ipc.h and
 * include/common/ipc_protocol.h.
 */
#include "arm7/ipc.h"
#include "arm7/irq.h"
#include "arm7/rtc.h"
#include "arm7/sdmmc.h"
#include "arm7/spi.h"
#include "common/dsi_hw.h"

/* @ref FUN_0001cc34  (address 0x01CC34) */
void ipc_fifo_wait_empty(void)
{
    while ((REG16(IPC_FIFOCNT) & IPC_FIFOCNT_SEND_EMPTY) == 0u) {
        /* spin until the send FIFO reports empty */
    }
}

void ipc_fifo_send_raw(u32 word)
{
    ipc_fifo_wait_empty();
    REG32(IPC_FIFOSEND) = word;
}

/*
 * @ref FUN_00020be4  (address 0x020BE4)
 *   FIFOSEND = payload | (channel << 28).
 */
void ipc_fifo_send(u32 channel, u32 payload)
{
    ipc_fifo_wait_empty();
    REG32(IPC_FIFOSEND) = ipc_pack(channel, payload);
}

void ipc_send_message(u32 channel, u32 payload)
{
    ipc_fifo_send(channel, payload);
}

bool ipc_fifo_try_recv(u32 *out)
{
    u32 cnt = REG16(IPC_FIFOCNT);

    if ((cnt & IPC_FIFOCNT_RECV_EMPTY) != 0u) {
        return false;
    }
    if (out != NULL) {
        *out = REG32(IPC_FIFORECV);
    } else {
        (void)REG32(IPC_FIFORECV);
    }
    return true;
}

/*
 * @ref FUN_0001cb38  (address 0x01CB38)
 *   Raise the sync IRQ line, wait for the low nibble to take, then commit.
 */
void ipc_sync_set(u32 value)
{
    value &= 0xFu;

    REG16(IPC_SYNC) = REG16(IPC_SYNC) | 0x4000u;   /* request sync IRQ */
    while ((REG16(IPC_SYNC) & 0xFu) != value) {
        /* wait for the peer/hardware to adopt the sent value */
    }
    REG16(IPC_SYNC) = (value << 8) | 0x6000u;
}

u32 ipc_sync_get(void)
{
    return REG16(IPC_SYNC);
}

/*
 * @ref FUN_0001f21c  (address 0x01F21C)
 *
 * The reference dispatcher is a large switch over the low 16 bits of each FIFO
 * word. This is the scaffolding: it drains the FIFO and routes by opcode. Each
 * case is filled in as the corresponding ARM7 service is translated.
 */

/* True when the DSi SCFG page is backed by hardware (TWL/DSi mode). */
static bool ipc_twl_mode(void)
{
    return (REG32(SCFG_EXT) & SCFG_EXT_SCFG_ACCESS) != 0u;
}

/*
 * IPC_CMD_GET_RTC — reply: status, date (y/m/d), time (h/m/s).
 * Uses the bit-banged RTC transport in src/arm7/rtc.c.
 */
static void ipc_reply_rtc(void)
{
    rtc_datetime_t dt = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    u32 status = 0u;

    if (rtc_read_datetime(&dt)) {
        status |= IPC_RTC_STATUS_VALID;
    }
    if ((dt.status & 0x40u) != 0u) {
        status |= IPC_RTC_STATUS_POWER_LOW;
    }
    if ((dt.status & 0x80u) != 0u) {
        status |= IPC_RTC_STATUS_POWER_OFF;
    }

    ipc_fifo_send(IPC_CH_SYSTEM,
                  ipc_reply_header(IPC_CMD_GET_RTC, IPC_REPLY_RTC_WORDS, status));
    ipc_fifo_send(IPC_CH_SYSTEM, ipc_pack_triple(dt.year, dt.month, dt.day));
    ipc_fifo_send(IPC_CH_SYSTEM, ipc_pack_triple(dt.hour, dt.minute, dt.second));
}

/*
 * IPC_CMD_GET_SDMMC_STATUS — reply: flags, raw TMIO status.
 * Reads only; never issues a command or writes to the card (AGENTS.md §5).
 */
static void ipc_reply_sdmmc(void)
{
    u32 flags = 0u;
    u32 raw = 0u;

    if (!ipc_twl_mode()) {
        /* The TMIO controller only exists on a DSi. */
        flags = IPC_SD_FLAG_UNAVAILABLE;
    } else {
        sdmmc_t c;
        sdmmc_status_t st;

        sdmmc_init_instance(&c, TMIO_SD_BASE, false);
        sdmmc_get_status(&c, &st);

        if (st.present) {
            flags |= IPC_SD_FLAG_PRESENT;
        }
        if (st.initialized) {
            flags |= IPC_SD_FLAG_INITIALIZED;
        }
        if (st.write_protected) {
            flags |= IPC_SD_FLAG_READONLY;
        }
        if (st.error) {
            flags |= IPC_SD_FLAG_ERROR;
        }
        raw = st.raw_status;
    }

    ipc_fifo_send(IPC_CH_SYSTEM,
                  ipc_reply_header(IPC_CMD_GET_SDMMC_STATUS, IPC_REPLY_SD_WORDS, 0u));
    ipc_fifo_send(IPC_CH_SYSTEM, flags);
    ipc_fifo_send(IPC_CH_SYSTEM, raw);
}

/*
 * IPC_CMD_GET_FW_INFO — reply: flags, JEDEC flash id.
 * The firmware SPI chip is present on both DS and DSi.
 */
static void ipc_reply_fw(void)
{
    u32 flags = 0u;
    u32 id = 0u;

    if (spi_firmware_id(&id)) {
        flags = IPC_FW_FLAG_OK;
    }

    ipc_fifo_send(IPC_CH_SYSTEM,
                  ipc_reply_header(IPC_CMD_GET_FW_INFO, IPC_REPLY_FW_WORDS, 0u));
    ipc_fifo_send(IPC_CH_SYSTEM, flags);
    ipc_fifo_send(IPC_CH_SYSTEM, id);
}

/*
 * IPC_CMD_GET_EXTKEY — reply: (active<<0), normalised EXTKEYIN bits.
 * EXTKEYIN (0x04000136) is ARM7-only; the ARM9 menu consumes this.
 */
static void ipc_reply_extkey(void)
{
    u16 raw = REG16(EXTKEYIN);
    u16 pressed = (u16)~raw;
    u32 keys = 0u;

    if ((pressed & EXTKEY_X) != 0u) {
        keys |= IPC_EXTKEY_X;
    }
    if ((pressed & EXTKEY_Y) != 0u) {
        keys |= IPC_EXTKEY_Y;
    }
    if ((pressed & EXTKEY_PEN) != 0u) {
        keys |= IPC_EXTKEY_PEN;
    }
    if ((pressed & EXTKEY_HINGE) != 0u) {
        keys |= IPC_EXTKEY_HINGE;
    }

    ipc_fifo_send(IPC_CH_SYSTEM,
                  ipc_reply_header(IPC_CMD_GET_EXTKEY, IPC_REPLY_EXTKEY_WORDS,
                                   IPC_EXTKEY_FLAG_ACTIVE));
    ipc_fifo_send(IPC_CH_SYSTEM, keys);
}

/* ------------------------------------------------------------------ */
/* ARM7-gated hardware delegation (AGENTS.md architectural rule)       */
/* ------------------------------------------------------------------ */

/*
 * Reply framing shared by the delegated read commands:
 *   word 0  reply header (opcode, IPC_REPLY_READ_WORDS, status = 0)
 *   word 1  result flags (IPC_IO_FLAG_*)
 *   word 2  completion count (sectors or bytes)
 */
static void ipc_reply_read(u32 cmd, u32 flags, u32 transferred)
{
    ipc_fifo_send(IPC_CH_SYSTEM,
                  ipc_reply_header(cmd, IPC_REPLY_READ_WORDS, 0u));
    ipc_fifo_send(IPC_CH_SYSTEM, flags);
    ipc_fifo_send(IPC_CH_SYSTEM, transferred);
}

/*
 * IPC_CMD_READ_SECTOR — read `count` 512-byte sectors from the requested TMIO
 * controller into the ARM9-supplied buffer in shared Main RAM.
 *
 * The ARM7 owns the TMIO register block (0x04004800 / 0x04004A00), so this is
 * the only place a sector read is issued. The eMMC path is read-only:
 * sdmmc_read() never writes and IPC_CMD_WRITE_SECTOR stays unimplemented
 * (AGENTS.md §5).
 *
 * Cache note: the ARM7TDMI has no data cache, so no ARM7-side maintenance is
 * required. The ARM9 is responsible for flushing the buffer before the request
 * and invalidating it after the reply (done in src/arm9/storage.c).
 */
static void ipc_handle_read_sector(const u32 *args, u32 nargs)
{
    /*
     * Persistent controller state: the SD card and the onboard eMMC share the
     * same TMIO controller (0x04004800) but are selected by port. Each instance
     * is opened (controller programmed) once, on first use.
     */
    static sdmmc_t s_sd;
    static sdmmc_t s_emmc;
    static bool s_sd_opened;
    static bool s_emmc_opened;

    u32 flags = 0u;
    u32 done = 0u;

    if (nargs < IPC_REQ_READ_SECTOR_WORDS) {
        ipc_reply_read(IPC_CMD_READ_SECTOR, IPC_IO_FLAG_BAD_RANGE, 0u);
        return;
    }

    {
        u32 dev   = args[0];
        u32 lba   = args[1];
        u32 dst   = args[2];
        u32 count = args[3];

        if (!ipc_twl_mode()) {
            /* The TMIO controller only exists on a DSi. */
            flags = IPC_IO_FLAG_UNAVAILABLE;
        } else if (count == 0u || count > IPC_STORAGE_MAX_SECTORS) {
            flags = IPC_IO_FLAG_BAD_RANGE;
        } else if (!ipc_addr_is_shared_ram(dst, count * IPC_STORAGE_SECTOR_SIZE)) {
            flags = IPC_IO_FLAG_BAD_ADDR;
        } else {
            bool emmc = (dev == IPC_STORAGE_DEV_EMMC);
            sdmmc_t *sc = emmc ? &s_emmc : &s_sd;
            bool *opened = emmc ? &s_emmc_opened : &s_sd_opened;

            if (!*opened) {
                sdmmc_init_instance(sc, TMIO_SD_BASE, emmc);
                (void)sdmmc_open(sc);
                *opened = true;
            }

            /*
             * Card-presence gate (read-only probe). Never issue a data command
             * into an empty slot: with an absent SD card the TMIO controller
             * leaves SIGSTATE clear and (e.g. melonDS "SENDING CMD %04X TO NULL
             * DEVICE") never raises CMDRESPEND, so a blind CMD17 would only spin
             * to the poll timeout and delay the reply. Report the slot as
             * unavailable so the ARM9 can fall back immediately. The onboard
             * eMMC is always populated, so it skips this gate.
             */
            if (!emmc) {
                sdmmc_status_t st;

                sdmmc_get_status(sc, &st);
                if (!st.present) {
                    flags = IPC_IO_FLAG_UNAVAILABLE;
                } else if (sdmmc_read(sc, lba * IPC_STORAGE_SECTOR_SIZE,
                                      (void *)(uintptr_t)dst,
                                      count * IPC_STORAGE_SECTOR_SIZE)) {
                    flags = IPC_IO_FLAG_OK;
                    done = count;
                } else {
                    flags = IPC_IO_FLAG_ERROR;
                }
            } else if (sdmmc_read(sc, lba * IPC_STORAGE_SECTOR_SIZE,
                                  (void *)(uintptr_t)dst,
                                  count * IPC_STORAGE_SECTOR_SIZE)) {
                flags = IPC_IO_FLAG_OK;
                done = count;
            } else {
                flags = IPC_IO_FLAG_ERROR;
            }
        }
    }

    ipc_reply_read(IPC_CMD_READ_SECTOR, flags, done);
}

/*
 * IPC_CMD_READ_FIRMWARE — read `len` bytes from the firmware SPI flash into the
 * ARM9-supplied buffer in shared Main RAM. SPI is an ARM7-only bus, so the
 * ARM9 reaches the flash exclusively through this request.
 */
static void ipc_handle_read_firmware(const u32 *args, u32 nargs)
{
    u32 flags = 0u;
    u32 done = 0u;

    if (nargs < IPC_REQ_READ_FIRMWARE_WORDS) {
        ipc_reply_read(IPC_CMD_READ_FIRMWARE, IPC_IO_FLAG_BAD_RANGE, 0u);
        return;
    }

    {
        u32 addr = args[0];
        u32 dst  = args[1];
        u32 len  = args[2];

        if (len == 0u || len > IPC_FW_MAX_BYTES) {
            flags = IPC_IO_FLAG_BAD_RANGE;
        } else if (!ipc_addr_is_shared_ram(dst, len)) {
            flags = IPC_IO_FLAG_BAD_ADDR;
        } else if (spi_firmware_read_buf(addr, (void *)(uintptr_t)dst, len)) {
            flags = IPC_IO_FLAG_OK;
            done = len;
        } else {
            flags = IPC_IO_FLAG_ERROR;
        }
    }

    ipc_reply_read(IPC_CMD_READ_FIRMWARE, flags, done);
}

/*
 * Route one fully-assembled command to its handler.
 *
 * @ref FUN_0001f21c  (address 0x01F21C) — the reference switch over the low 16
 * bits of each command word.
 */
static void ipc_dispatch_command(u32 opcode, const u32 *args, u32 nargs)
{
    switch (opcode) {
    case IPC_CMD_PING:
        /* Boot handshake: acknowledge so the ARM9 can turn its status
         * indicator green. */
        ipc_fifo_send(IPC_CH_SYSTEM, IPC_CMD_PONG);
        break;
    case IPC_CMD_GET_RTC:
        ipc_reply_rtc();
        break;
    case IPC_CMD_GET_SDMMC_STATUS:
        ipc_reply_sdmmc();
        break;
    case IPC_CMD_GET_FW_INFO:
        ipc_reply_fw();
        break;
    case IPC_CMD_GET_EXTKEY:
        ipc_reply_extkey();
        break;
    case IPC_CMD_READ_SECTOR:
        ipc_handle_read_sector(args, nargs);
        break;
    case IPC_CMD_READ_FIRMWARE:
        ipc_handle_read_firmware(args, nargs);
        break;
    case IPC_CMD_SET_STATE:
    case IPC_CMD_QUERY_STATE:
    case IPC_CMD_HANDLE_BLOCK:
    case IPC_CMD_GET_INFO:
    case IPC_CMD_WRITE_SECTOR:
    case IPC_CMD_STATUS:
    case IPC_CMD_RESUME:
    case IPC_CMD_ARM9_INIT:
    case IPC_CMD_FLUSH:
    case IPC_CMD_RAW_SWI:
    case IPC_CMD_FS_OPEN:
    case IPC_CMD_FS_SEEK:
    default:
        /* TODO(re): port each case from FUN_0001f21c. IPC_CMD_WRITE_SECTOR in
         * particular must stay unimplemented until ENABLE_NAND_WRITE_DANGER
         * gates a reviewed write path (AGENTS.md §5). */
        break;
    }
}

/*
 * Parameterised-request assembler. A request is a header word whose bits 16-19
 * give the number of argument words that follow, then that many words. Legacy
 * single-word commands have a count of zero and are dispatched immediately.
 */
typedef struct {
    bool active;
    u32  opcode;
    u32  expected;
    u32  count;
    u32  args[IPC_REQUEST_MAX_WORDS];
} ipc_request_t;

static ipc_request_t s_request;

void ipc_dispatch_pending(void)
{
    u32 word;
    u32 saved = irq_disable_all();

    while (ipc_fifo_try_recv(&word)) {
        if (!s_request.active) {
            u32 payload = ipc_payload(word);
            u32 opcode = payload & 0xFFFFu;
            u32 words = ipc_request_words(payload);

            if (words == 0u) {
                ipc_dispatch_command(opcode, NULL, 0u);
            } else {
                s_request.active = true;
                s_request.opcode = opcode;
                s_request.expected = words;
                s_request.count = 0u;
            }
        } else {
            if (s_request.count < IPC_REQUEST_MAX_WORDS) {
                s_request.args[s_request.count] = word;
            }
            s_request.count++;
            if (s_request.count >= s_request.expected) {
                ipc_dispatch_command(s_request.opcode, s_request.args,
                                     s_request.expected);
                s_request.active = false;
                s_request.count = 0u;
            }
        }
    }

    irq_restore(saved);
}
