/*
 * include/arm7/ipc.h
 *
 * ARM7 side of the DSi IPC FIFO bridge.
 *
 * Derived from FUN_0001cc34, FUN_0001cb38, FUN_00020be4 and the command
 * dispatcher FUN_0001f21c (see docs/hardware_notes.md §3 and
 * include/common/ipc_protocol.h).
 */
#ifndef DSI_ARM7_IPC_H
#define DSI_ARM7_IPC_H

#include "../common/types.h"
#include "../common/ipc_protocol.h"

/*
 * Busy-wait until the IPC send FIFO is empty.
 *
 * @ref FUN_0001cc34  (address 0x01CC34)
 *   Polls IPCFIFOCNT bit 1 until clear. Blocks with interrupts disabled in the
 *   reference implementation, so callers must already be in a critical
 *   section if they cannot tolerate long stalls.
 */
void ipc_fifo_wait_empty(void);

/*
 * Push one raw 32-bit word into the send FIFO (assumes FIFO already empty).
 *
 * @ref FUN_0001cc34 writes the fixed word 0x40000002.
 */
void ipc_fifo_send_raw(u32 word);

/*
 * Send a channel-tagged message: payload | (channel << 28).
 *
 * @ref FUN_00020be4  (address 0x020BE4)
 *   r0 = channel/source nibble, r1 = payload.
 */
void ipc_fifo_send(u32 channel, u32 payload);

/*
 * Convenience wrapper around ipc_fifo_wait_empty() + ipc_fifo_send().
 */
void ipc_send_message(u32 channel, u32 payload);

/*
 * Non-blocking receive of one FIFO word.
 * Returns true and stores the word if data was available.
 */
bool ipc_fifo_try_recv(u32 *out);

/*
 * Set the low nibble of IPCSYNC (the ARM9->ARM7 synchronisation flags) and
 * raise the sync IRQ line (bit 14).
 *
 * @ref FUN_0001cb38  (address 0x01CB38)
 *   Sets IPCSYNC |= 0x4000, waits until the low nibble equals the requested
 *   value, then writes (value << 8) | 0x6000.
 */
void ipc_sync_set(u32 value);

/* Read the current IPCSYNC value. */
u32 ipc_sync_get(void);

/*
 * ARM7 command dispatcher for messages arriving from ARM9.
 *
 * @ref FUN_0001f21c  (address 0x01F21C)
 *   IDLE -> drain FIFO -> decode opcode -> handle -> ack. This entry point is
 *   called from the main ARM7 loop (and the IPC-receive IRQ); it is NOT an
 *   exception handler.
 *
 * Both legacy single-word commands and parameterised requests are accepted. A
 * parameterised request is a header built with ipc_request_header(cmd, n) and
 * followed by `n` argument words; the dispatcher assembles the whole request
 * (across one or more calls, if the words arrive separately) before running the
 * handler. See include/common/ipc_protocol.h for the request/reply layouts.
 */
void ipc_dispatch_pending(void);

#endif /* DSI_ARM7_IPC_H */
