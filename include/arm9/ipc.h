/*
 * include/arm9/ipc.h
 *
 * ARM9 side of the IPC bridge.
 *
 * Translated from the reference ARM9 image:
 *   @ref FUN_0602c008 (0x0602C008)  wait until the send FIFO is empty
 *   @ref FUN_0602bff0 (0x0602BFF0)  send_fifo = payload | (channel << 28)
 *   @ref crt0         (0x02200800)  IPCSYNC handshake before the VRAM handoff
 *   @ref entry        (0x0602572C)  IPCFIFOCNT = 0x101 (enable + clear)
 */
#ifndef DSI_ARM9_IPC_H
#define DSI_ARM9_IPC_H

#include "../common/types.h"

/* Wait until the IPC send FIFO reports empty (@ref FUN_0602c008). */
void arm9_ipc_wait_send_empty(void);

/*
 * Enable the IPC FIFO hardware and clear both directions.
 * The reference writes 0x101; we additionally set the master-enable bit so the
 * FIFO works under emulation as well (value in dsi_hw.h).
 */
void arm9_ipc_fifo_init(void);

/*
 * Push one word onto the send FIFO, tagged with the logical channel in the
 * high nibble. @ref FUN_0602bff0.
 */
void arm9_ipc_send(u32 channel, u32 payload);

/*
 * Non-blocking send: returns false (without blocking) if the send FIFO is not
 * empty. Used by the boot handshake so a dead peer cannot wedge the ARM9.
 */
bool arm9_ipc_try_send(u32 channel, u32 payload);

/* Non-blocking receive: returns true and stores the word if one is queued. */
bool arm9_ipc_try_recv(u32 *out);

/*
 * Synchronous request/response transaction on the system channel.
 *
 * Sends `cmd`, then polls the receive FIFO until the matching reply header
 * (low 16 bits == cmd) and its payload words have arrived, or `timeout` poll
 * iterations elapse. Stray words (e.g. an outstanding PONG) are discarded.
 *
 * `reply[0]` receives the header (see ipc_reply_header()); following words
 * receive the payload. Returns the number of words stored in `reply`
 * (0 on timeout). `max_words` bounds the write into `reply`.
 */
u32 arm9_ipc_query(u32 channel, u32 cmd, u32 *reply, u32 max_words,
                   u32 timeout);

/*
 * Parameterised variant: sends ipc_request_header(cmd, nargs) followed by
 * `nargs` argument words before waiting for the reply. This is how the ARM9
 * asks the ARM7 to perform an operation on an ARM7-only controller (e.g. a
 * TMIO sector read or a firmware SPI read); see src/arm9/storage.c.
 */
u32 arm9_ipc_query_args(u32 channel, u32 cmd, const u32 *args, u32 nargs,
                        u32 *reply, u32 max_words, u32 timeout);

/*
 * Wait until IPCSYNC's low nibble reads `value`, having first raised the sync
 * IRQ line. Mirrors the handshake at @ref crt0 0x02200818..0x02200830:
 *
 *     while ((IPCSYNC & 0xF) != 0xE) ;
 *     IPCSYNC = 0x4E00;
 */
void arm9_ipc_sync_wait(u32 value);

/* Read the raw IPCSYNC register. */
u32 arm9_ipc_sync_read(void);

#endif /* DSI_ARM9_IPC_H */
