/*
 * include/common/ipc_protocol.h
 *
 * Logical ARM9 <-> ARM7 protocol carried over the DSi IPC FIFO.
 *
 * The ARM7-side dispatcher was recovered as FUN_0001f21c (see
 * docs/hardware_notes.md §3). FUN_00020be4() writes
 *
 *     send_fifo = payload | (channel << 28)
 *
 * so the high nibble selects a logical channel and the low 28 bits carry the
 * command/argument. The command words the reference dispatcher switches on are
 * listed below. Names are descriptive; the numeric values are exact.
 */
#ifndef DSI_COMMON_IPC_PROTOCOL_H
#define DSI_COMMON_IPC_PROTOCOL_H

#include "types.h"

/* ---- Channel (high nibble of the FIFO word) ---- */
#define IPC_CH_SYSTEM   0x0u
#define IPC_CH_FS       0x1u
#define IPC_CH_UNKNOWN  0x3u

/* ---- System channel opcodes (FUN_0001f21c switch targets) ---- */
#define IPC_CMD_PING            0x1001u
#define IPC_CMD_SET_STATE       0x1002u
#define IPC_CMD_QUERY_STATE     0x1003u
#define IPC_CMD_HANDLE_BLOCK    0x1004u   /* firmware/partition block    */
#define IPC_CMD_GET_INFO        0x1006u
#define IPC_CMD_READ_SECTOR     0x1007u   /* disk read                   */
#define IPC_CMD_WRITE_SECTOR    0x1008u   /* disk write (gated by policy) */
#define IPC_CMD_STATUS          0x100Au
#define IPC_CMD_RESUME          0x100Du
#define IPC_CMD_ARM9_INIT       0x1010u
#define IPC_CMD_FLUSH           0x1025u
#define IPC_CMD_RAW_SWI         0x000Eu   /* forward one BIOS SWI         */
#define IPC_CMD_FS_OPEN         0x3007u
#define IPC_CMD_FS_SEEK         0x3008u

/*
 * ---- Telemetry commands added by the C99 bootloader --------------------
 *
 * These are request/response transactions. The request is a single word on
 * the system channel; the response is a header word (built with
 * ipc_reply_header()) followed by `words` payload words.
 *
 * The opcode values were chosen from the gaps in the reference dispatcher's
 * switch table so they cannot collide with FUN_0001f21c cases.
 */
#define IPC_CMD_GET_RTC         0x100Bu   /* -> status, date, time          */
#define IPC_CMD_GET_SDMMC_STATUS 0x100Cu  /* -> flags, raw status, card info */
#define IPC_CMD_GET_FW_INFO     0x100Eu   /* -> flags, JEDEC id             */
#define IPC_CMD_GET_EXTKEY      0x100Fu   /* -> (flags<<16)|extkey bits     */

/*
 * ---- ARM7-gated hardware delegation (architectural rule) -----
 *
 * The TMIO SD/eMMC controller and the firmware SPI flash are reachable only
 * from the ARM7. The ARM9 never touches their registers: it packages the
 * operation into a parameterised request (see ipc_request_header()) and waits
 * for the ARM7 reply. The reply carries a result-flag word and a completion
 * count; see IPC_REPLY_READ_WORDS below.
 *
 * IPC_CMD_READ_SECTOR request payload (IPC_REQ_READ_SECTOR_WORDS words):
 *   arg0  device      IPC_STORAGE_DEV_*
 *   arg1  lba         first 512-byte sector
 *   arg2  dst         destination buffer address in shared Main RAM
 *   arg3  count       number of 512-byte sectors
 *
 * IPC_CMD_READ_FIRMWARE request payload (IPC_REQ_READ_FIRMWARE_WORDS words):
 *   arg0  address     24-bit firmware byte address
 *   arg1  dst         destination buffer address in shared Main RAM
 *   arg2  length      number of bytes (<= IPC_FW_MAX_BYTES)
 */
#define IPC_CMD_READ_FIRMWARE   0x1011u   /* -> flags, bytes transferred    */

/*
 * ---- Chainload handoff (dual-CPU) --------------------------------------
 *
 * The ARM9 stages the selected image and asks the ARM7 to take the handoff.
 * The ARM7 copies the staged ARM7 payload from shared Main RAM to its final
 * address (Main RAM or ARM7-private WRAM) and then, when the boot guard is
 * armed, branches to it.
 *
 * IPC_CMD_BOOT_HANDOFF request payload (IPC_REQ_BOOT_HANDOFF_WORDS words):
 *   arg0  arm7_entry  final ARM7 entry point
 *   arg1  arm7_src    staged ARM7 payload address in shared Main RAM
 *   arg2  arm7_dst    final ARM7 load address
 *   arg3  arm7_size   number of bytes to copy
 *
 * Reply (IPC_REPLY_BOOT_WORDS words):
 *   word 0  reply header
 *   word 1  IPC_BOOT_FLAG_* result
 */
#define IPC_CMD_BOOT_HANDOFF    0x1012u

/*
 * ---- Guarded NAND write path (ENABLE_NAND_WRITE_DANGER only) ------------
 *
 * The whole write path exists only in a build that defines
 * ENABLE_NAND_WRITE_DANGER (the project conventions). Even there, the ARM7 refuses to
 * write the onboard eMMC until it has been explicitly armed:
 *
 * IPC_CMD_NAND_GUARD request payload (IPC_REQ_NAND_GUARD_WORDS words):
 *   arg0  auth_id   0 = disarm; non-zero = arm with the token-derived id
 *
 * IPC_CMD_WRITE_SECTOR request payload (IPC_REQ_WRITE_SECTOR_WORDS words):
 *   arg0  device      IPC_STORAGE_DEV_* (SD backup sink / guarded eMMC)
 *   arg1  lba         first 512-byte sector
 *   arg2  src         source buffer address in shared Main RAM
 *   arg3  count       number of 512-byte sectors
 *
 * The reply reuses the IPC_REPLY_READ_WORDS framing (flags + transferred).
 */
#define IPC_CMD_NAND_GUARD      0x1013u

/* Request payload word counts (excluding the request header). */
#define IPC_REQ_READ_SECTOR_WORDS   4u
#define IPC_REQ_READ_FIRMWARE_WORDS 3u
#define IPC_REQ_BOOT_HANDOFF_WORDS  4u
#define IPC_REQ_WRITE_SECTOR_WORDS  4u
#define IPC_REQ_NAND_GUARD_WORDS    1u

/* Largest request we will assemble (bounds the ARM7-side argument buffer). */
#define IPC_REQUEST_MAX_WORDS       4u

/* IPC_CMD_BOOT_HANDOFF result flags (reply payload word 0). */
#define IPC_BOOT_FLAG_ACCEPTED  BIT(0)   /* handoff request recorded       */
#define IPC_BOOT_FLAG_DRYRUN    BIT(1)   /* built without the boot guard   */
#define IPC_BOOT_FLAG_BAD_ADDR  BIT(2)   /* staged payload not in shared RAM */

/* Storage device selector for IPC_CMD_READ_SECTOR. */
#define IPC_STORAGE_DEV_SD      0u
#define IPC_STORAGE_DEV_EMMC    1u

/* Per-transfer bounds so a single request cannot monopolise the ARM7. */
#define IPC_STORAGE_SECTOR_SIZE 512u
#define IPC_STORAGE_MAX_SECTORS 32u   /* 16 KiB */
#define IPC_FW_MAX_BYTES        256u

/*
 * Result flags shared by the delegated read commands (reply payload word 0).
 */
#define IPC_IO_FLAG_OK          BIT(0)
#define IPC_IO_FLAG_ERROR       BIT(1)
#define IPC_IO_FLAG_BAD_ADDR    BIT(2)   /* buffer not in shared Main RAM   */
#define IPC_IO_FLAG_BAD_RANGE   BIT(3)   /* count/length out of range       */
#define IPC_IO_FLAG_UNAVAILABLE BIT(4)   /* controller absent (legacy DS)   */
#define IPC_IO_FLAG_LOCKED      BIT(5)   /* eMMC write guard not armed      */

/* Payload word counts (excluding the reply header). */
#define IPC_REPLY_RTC_WORDS     3u
#define IPC_REPLY_SD_WORDS      3u   /* flags, raw status, card info       */
#define IPC_REPLY_FW_WORDS      2u
#define IPC_REPLY_EXTKEY_WORDS  1u
#define IPC_REPLY_READ_WORDS    2u   /* flags, transferred (sectors/bytes) */
#define IPC_REPLY_BOOT_WORDS    1u   /* handoff result flags               */
#define IPC_REPLY_NAND_GUARD_WORDS 1u /* guard result flags                */

/* IPC_CMD_GET_RTC status bits (reply header status field). */
#define IPC_RTC_STATUS_VALID     BIT(0)
#define IPC_RTC_STATUS_POWER_LOW BIT(1)
#define IPC_RTC_STATUS_POWER_OFF BIT(2)

/* IPC_CMD_GET_SDMMC_STATUS flag bits (payload word 0). */
#define IPC_SD_FLAG_PRESENT      BIT(0)
#define IPC_SD_FLAG_INITIALIZED  BIT(1)
#define IPC_SD_FLAG_READONLY     BIT(2)
#define IPC_SD_FLAG_ERROR        BIT(3)
#define IPC_SD_FLAG_UNAVAILABLE  BIT(4)   /* legacy DS: no TMIO controller */

/*
 * IPC_CMD_GET_SDMMC_STATUS card-info bits (payload word 2). Populated from the
 * OCR CCS bit once the read-only identification sequence has run; 0 while the
 * card is absent or has not been identified yet.
 */
#define IPC_SD_INFO_BLOCK_ADDRESSED BIT(0) /* SDHC/SDXC (block-addressed)  */
#define IPC_SD_INFO_SDHC            IPC_SD_INFO_BLOCK_ADDRESSED
#define IPC_SD_INFO_CARD_READY      BIT(1) /* reached the transfer state    */

/* IPC_CMD_GET_FW_INFO flag bits (payload word 0). */
#define IPC_FW_FLAG_OK           BIT(0)
#define IPC_FW_FLAG_UNAVAILABLE  BIT(1)

/* IPC_CMD_GET_EXTKEY flag bits (payload word 0). */
#define IPC_EXTKEY_FLAG_ACTIVE   BIT(0)

/* IPC_CMD_GET_EXTKEY normalised key bits (payload word 1). */
#define IPC_EXTKEY_X      BIT(0)
#define IPC_EXTKEY_Y      BIT(1)
#define IPC_EXTKEY_PEN    BIT(2)
#define IPC_EXTKEY_HINGE  BIT(3)

/*
 * Boot handshake: the ARM9 sends IPC_CMD_PING on the system channel and waits
 * for IPC_CMD_PONG before it continues. Used by the melonDS smoke test to
 * prove the two cores can talk over the FIFO.
 */
#define IPC_CMD_PONG            0x1000u

/*
 * FIFO word layout. The dispatcher reads a 16-bit "length" field and the
 * byte-justified fields directly out of the payload; this mirrors that view.
 */
typedef struct {
    u32 word;      /* raw FIFO word = payload | (channel << 28)            */
} ipc_message_t;

static inline u32 ipc_pack(u32 channel, u32 payload)
{
    return (payload & 0x0FFFFFFFu) | ((channel & 0xFu) << 28);
}

static inline u32 ipc_channel(u32 word)
{
    return (word >> 28) & 0xFu;
}

static inline u32 ipc_payload(u32 word)
{
    return word & 0x0FFFFFFFu;
}

/*
 * Request header layout for parameterised commands:
 *
 *   bits  0-15  opcode
 *   bits 16-19  number of argument words that follow the header
 *   bits 20-27  reserved (must be 0)
 *   bits 28-31  reserved; the channel nibble is added by ipc_pack()
 *
 * A legacy single-word request is simply a header with `words == 0`, so
 * ipc_request_header(cmd, 0) == cmd and existing commands keep working.
 */
static inline u32 ipc_request_header(u32 cmd, u32 words)
{
    return (cmd & 0xFFFFu) | ((words & 0xFu) << 16);
}

static inline u32 ipc_request_words(u32 header)
{
    return (header >> 16) & 0xFu;
}

/*
 * Shared-buffer window. A delegated transfer may only name a destination in
 * the low 4 MiB of Main RAM, which both cores can reach in DS and DSi modes.
 * This deliberately excludes DTCM (0x0B000000, ARM9-only and not DMA-safe) and
 * the VRAM/I/O pages, so a malformed request cannot make the ARM7 scribble over
 * its own image or over MMIO.
 */
#define IPC_SHARED_RAM_BASE 0x02000000u
#define IPC_SHARED_RAM_END  0x02400000u

static inline bool ipc_addr_is_shared_ram(u32 addr, u32 len)
{
    u32 size = IPC_SHARED_RAM_END - IPC_SHARED_RAM_BASE;

    if (len == 0u || len > size) {
        return false;
    }
    if (addr < IPC_SHARED_RAM_BASE) {
        return false;
    }
    /* addr + len <= END, arranged to avoid unsigned overflow. */
    return addr <= IPC_SHARED_RAM_END - len;
}

/*
 * Response header layout for the telemetry commands:
 *
 *   bits  0-15  echoed opcode
 *   bits 16-19  number of payload words that follow
 *   bits 20-27  status flags
 *   bits 28-31  reserved (channel nibble is added by ipc_pack)
 */
static inline u32 ipc_reply_header(u32 cmd, u32 words, u32 status)
{
    return (cmd & 0xFFFFu) |
           ((words & 0xFu) << 16) |
           ((status & 0xFFu) << 20);
}

static inline u32 ipc_reply_words(u32 header)
{
    return (header >> 16) & 0xFu;
}

static inline u32 ipc_reply_status(u32 header)
{
    return (header >> 20) & 0xFFu;
}

/* Pack an 8/8/8 date or time triple into one payload word. */
static inline u32 ipc_pack_triple(u8 a, u8 b, u8 c)
{
    return ((u32)a << 16) | ((u32)b << 8) | (u32)c;
}

static inline u8 ipc_unpack_triple_hi(u32 word) { return (u8)(word >> 16); }
static inline u8 ipc_unpack_triple_mid(u32 word) { return (u8)(word >> 8); }
static inline u8 ipc_unpack_triple_lo(u32 word) { return (u8)word; }

#endif /* DSI_COMMON_IPC_PROTOCOL_H */
