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
#include "../common/dsi_hw.h"
#include "../common/nand.h"
#include "../common/nds_header.h"
#include "boot_cfg.h"
#include "input.h"

/* Menu entries, in display order. */
#define MENU_ITEM_COUNT   9u

/* Maximum number of .nds/.srl files listed by the SD browser. */
#define MENU_BROWSE_MAX   16u

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_SELFTEST,   /* [1] re-run the IPC/hardware self-test      */
    MENU_ACTION_INSPECT,    /* [2] dump SCFG/MBK registers                */
    MENU_ACTION_SD,         /* [3] probe the SD/TMIO controller           */
    MENU_ACTION_LZ77,       /* [4] expand the embedded LZ77 self-test     */
    MENU_ACTION_HALT,       /* [5] halt the system                        */
    MENU_ACTION_BROWSE,     /* [6] browse the SD card's FAT volume        */
    MENU_ACTION_SCFG_INIT,  /* [7] unlock + initialise SCFG/MBK           */
    MENU_ACTION_AUTOBOOT,   /* [8] autoboot BOOT.NDS from the SD root     */
    MENU_ACTION_INSPECT_CFG /* [9] inspect BOOT.CFG + hotkey bindings     */
} menu_action_t;

typedef struct {
    u8 cursor;   /* 0..count-1 */
    u8 count;
} menu_state_t;

/* Snapshot of the live hardware telemetry shown on the top screen. */
typedef struct {
    bool twl_mode;
    bool scfg_accessible;   /* the DSi-only 0x04004000 page is present     */
    bool extended_ram;      /* Main RAM window is 16/32 MiB                */
    bool ndma;              /* NDMA engine accessible                      */
    bool scfg_unlocked;     /* SCFG_EXT.bit31 (SCFG/MBK access)            */
    u32  a9rom;
    u32  scfg_clk;
    u32  scfg_ext;
    u32  arm9_hz;           /* decoded ARM9 core clock                     */
    u32  mbk[MBK_COUNT];    /* raw MBK1..MBK9                              */
    u32  mpu_control;
    u32  dtcm_base;
    bool ipc_ok;
    u32  ipc_ping_count;
    bool rtc_valid;
    u8   year, month, day, hour, minute, second;
    u8   sd_flags;
    u32  sd_raw;
    u8   sd_card_type;    /* 0=SDSC/unknown, 1=SDHC/SDXC (block-addressed) */
    bool sd_read_ok;      /* delegated LBA-0 read via ARM7 succeeded     */
    bool sd_mbr_ok;       /* the read sector carries the 0x55AA MBR sig  */
    u8   sd_fs_type;      /* fat_type_t of the card volume (0 = none)    */
    u32  sd_total_sectors;/* card volume size in 512-byte sectors        */
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

    /* Chainloader dry-run / staging state (item [6]). */
    bool loader_inspected;  /* Y/SELECT ran a verify/dry-run inspect     */
    u8   loader_status;     /* nds_load_status_t of the last inspect     */
    bool loader_loadable;   /* loader_status == NDS_LOAD_OK              */
    u8   loader_unit_code;  /* raw 0x012 unit code of the inspected file */
    u32  loader_arm9_entry;
    u32  loader_arm9_load;
    u32  loader_arm9_size;
    u32  loader_arm7_entry;
    u32  loader_arm7_load;
    u32  loader_arm7_size;
    bool boot_staged;       /* A staged the image (dry run or real)      */
    bool boot_dry_run;      /* staging ran but no CPU handoff happened   */

    /* DLDI section report for the [6] verify/dry-run inspector. */
    bool dldi_present;      /* a valid DLDI stub was found in the ARM9   */
    bool dldi_patched;      /* it was replaced by the built-in RO driver */
    u32  dldi_alloc_kb;     /* reserved block size in KiB                */
    char dldi_io_type[5];   /* 4-char IOType tag + NUL                   */

    /* NDS banner report for the [6] verify/dry-run inspector. */
    bool banner_present;    /* a banner was read and its version accepted */
    bool banner_crc_ok;     /* the banner CRC16 verified                  */
    char banner_title[NDS_BANNER_TITLE_MAX + 1u]; /* decoded English title */

    /* Unlaunch-style BOOT.NDS / BOOT.CFG autoboot policy / discovery state. */
    bool autoboot_checked;  /* the SD root was probed for the target       */
    bool autoboot_ready;    /* the resolved target was found               */
    bool autoboot_key_held; /* B/SELECT forced the menu at startup         */
    bool boot_cfg_present;  /* a BOOT.CFG was found on the SD root         */
    char autoboot_name[BOOT_CFG_PATH_MAX]; /* the resolved target path     */
    char autoboot_sel[8];   /* "[...]" tag: "CFG", "A", "X", ... or ""     */

    /* Boot theme / splash state (see include/arm9/theme.h). */
    bool theme_applied;     /* the BOOT.CFG theme (or defaults) was applied */
    bool splash_loaded;     /* a valid SPLASH.RAW was read + presented      */
    u8   splash_status;     /* theme_splash_status_t of the last attempt    */

    /* NAND / HiyaCFW redirect state (read-only eMMC probe). */
    bool nand_ok;            /* the NAND MBR was read and parsed           */
    bool nand_fat;           /* a FAT partition was located                */
    bool nand_launcher;      /* the system launcher was resolved on NAND   */
    u8   nand_format;        /* nand_table_format_t of the decoded table   */
    u32  nand_partitions;
    u32  nand_fat_lba;
    u32  nand_total_sectors;
    char nand_launcher_path[NAND_LAUNCHER_PATH_MAX];

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
