/*
 * src/arm9/ipc.c
 *
 * ARM9 IPC bridge. See include/arm9/ipc.h.
 */
#include "arm9/ipc.h"
#include "common/dsi_hw.h"
#include "common/ipc_protocol.h"

/* @ref FUN_0602c008 (0x0602C008). */
void arm9_ipc_wait_send_empty(void)
{
    while ((REG16(IPC_FIFOCNT) & IPC_FIFOCNT_SEND_EMPTY) == 0u) {
        /* spin */
    }
}

/*
 * @ref entry (0x0602572C): IPCFIFOCNT = 0x101.
 * The reference relies on the firmware having set the master-enable bit; we
 * set it explicitly (enable + send-enable + flush RX) so the FIFO also works
 * under emulation.
 */
void arm9_ipc_fifo_init(void)
{
    REG16(IPC_FIFOCNT) = IPC_FIFOCNT_INIT_VALUE;
}

/* @ref FUN_0602bff0 (0x0602BFF0). */
void arm9_ipc_send(u32 channel, u32 payload)
{
    arm9_ipc_wait_send_empty();
    REG32(IPC_FIFOSEND) = ipc_pack(channel, payload);
}

bool arm9_ipc_try_send(u32 channel, u32 payload)
{
    if ((REG16(IPC_FIFOCNT) & IPC_FIFOCNT_SEND_EMPTY) == 0u) {
        return false;
    }
    REG32(IPC_FIFOSEND) = ipc_pack(channel, payload);
    return true;
}

bool arm9_ipc_try_recv(u32 *out)
{
    if ((REG16(IPC_FIFOCNT) & IPC_FIFOCNT_RECV_EMPTY) != 0u) {
        return false;
    }
    {
        u32 word = REG32(IPC_FIFORECV);
        if (out != NULL) {
            *out = word;
        }
    }
    return true;
}

/*
 * Synchronous request/response transaction. See include/arm9/ipc.h.
 *
 * The request is a header (ipc_request_header) followed by `nargs` argument
 * words; the reply header carries the payload word count in bits 16-19, so the
 * caller does not need to know the size of each command's reply.
 */
u32 arm9_ipc_query_args(u32 channel, u32 cmd, const u32 *args, u32 nargs,
                        u32 *reply, u32 max_words, u32 timeout)
{
    u32 got = 0u;
    u32 want = 0u;
    bool have_header = false;
    u32 i;

    arm9_ipc_send(channel, ipc_request_header(cmd, nargs));
    for (i = 0u; i < nargs; i++) {
        arm9_ipc_send(channel, (args != NULL) ? args[i] : 0u);
    }

    for (i = 0u; i < timeout; i++) {
        u32 word;

        if (!arm9_ipc_try_recv(&word)) {
            continue;
        }

        if (!have_header) {
            if ((word & 0xFFFFu) != (cmd & 0xFFFFu)) {
                continue;   /* discard a stray reply from an earlier request */
            }
            have_header = true;
            want = ipc_reply_words(word);
            if (reply != NULL && got < max_words) {
                reply[got] = word;
            }
            got++;
            if (want == 0u) {
                return got;
            }
            continue;
        }

        if (reply != NULL && got < max_words) {
            reply[got] = word;
        }
        got++;
        if (got >= want + 1u) {
            return got;
        }
    }

    return have_header ? got : 0u;
}

u32 arm9_ipc_query(u32 channel, u32 cmd, u32 *reply, u32 max_words,
                   u32 timeout)
{
    return arm9_ipc_query_args(channel, cmd, NULL, 0u, reply, max_words,
                               timeout);
}

/* @ref crt0 (0x02200818..0x02200830). */
void arm9_ipc_sync_wait(u32 value)
{
    value &= 0xFu;
    while ((REG16(IPC_SYNC) & 0xFu) != value) {
        /* spin until the peer has adopted the sent nibble */
    }
    REG16(IPC_SYNC) = 0x4E00u;
}

u32 arm9_ipc_sync_read(void)
{
    return REG16(IPC_SYNC);
}
