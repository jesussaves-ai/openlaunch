/*
 * include/arm9/menu.h
 *
 * Interactive boot menu and live hardware diagnostics for the top/bottom
 * screens. The menu state machine and its action mapping are pure and
 * host-testable; menu_run() drives the hardware.
 */
#ifndef DSI_ARM9_MENU_H
#define DSI_ARM9_MENU_H

#include "../common/types.h"
#include "input.h"

/* Menu entries, in display order. */
#define MENU_ITEM_COUNT   6u

/* Maximum number of .nds/.srl files listed by the SD browser. */
#define MENU_BROWSE_MAX   16u

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_SELFTEST,   /* [1] re-run the IPC/hardware self-test      */
    MENU_ACTION_INSPECT,    /* [2] dump SCFG/MBK registers                */
    MENU_ACTION_SD,         /* [3] probe the SD/TMIO controller           */
    MENU_ACTION_LZ77,       /* [4] expand the embedded LZ77 self-test     */
    MENU_ACTION_HALT,       /* [5] halt the system                        */
    MENU_ACTION_BROWSE       /* [6] browse the SD card's FAT volume        */
} menu_action_t;

typedef struct {
    u8 cursor;   /* 0..count-1 */
    u8 count;
} menu_state_t;

/* Snapshot of the live hardware telemetry shown on the top screen. */
typedef struct {
    bool twl_mode;
    u32  a9rom;
    u32  scfg_clk;
    u32  scfg_ext;
    u32  mpu_control;
    u32  dtcm_base;
    bool ipc_ok;
    u32  ipc_ping_count;
    bool rtc_valid;
    u8   year, month, day, hour, minute, second;
    u8   sd_flags;
    u32  sd_raw;
    bool sd_read_ok;      /* delegated LBA-0 read via ARM7 succeeded     */
    bool sd_mbr_ok;       /* the read sector carries the 0x55AA MBR sig  */
    bool fw_ok;
    u32  fw_id;
    bool fw_read_ok;      /* delegated SPI read via ARM7 (IPC) succeeded */
    u32  fw_read_word;    /* first four bytes read from the firmware     */
    bool lz77_ok;
    u32  lz77_bytes;

    bool fat_ok;          /* the SD FAT volume mounted                   */
    bool boot_loaded;     /* a .nds header was read from the card        */
    bool boot_crc_ok;     /* that header's CRC16 verified                */
    char boot_title[13];  /* NDS title (12 chars + NUL)                  */
    char boot_code[5];    /* NDS game code (4 chars + NUL)               */

    const char *last_action;
} menu_diag_t;

/* ---- Pure menu state machine (host-testable) -------------------------- */

void menu_state_init(menu_state_t *m, u8 count);

/* Move the cursor by `delta`, wrapping around. */
void menu_state_move(menu_state_t *m, s32 delta);

u8 menu_state_selected(const menu_state_t *m);

/* Map the current cursor position to an action. */
menu_action_t menu_action_for(const menu_state_t *m);

/* ---- Hardware driver -------------------------------------------------- */

/* Initialise the console, run the self-test, then service the menu forever. */
void menu_run(void) __attribute__((noreturn));

#endif /* DSI_ARM9_MENU_H */
