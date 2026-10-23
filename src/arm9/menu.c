/*
 * src/arm9/menu.c
 *
 * Interactive boot menu + live hardware diagnostics. See include/arm9/menu.h.
 *
 * Top screen (Engine A): header and live telemetry.
 * Bottom screen (Engine B): D-Pad navigable menu, A to run, B to re-run the
 * self-test.
 *
 * All hardware access is read-only except for HALTCNT on the explicit
 * "[5] Reboot / Halt System" action; the eMMC/NAND guard (the project conventions) is
 * reported from the build configuration.
 */
#include "arm9/menu.h"
#include "arm9/boot.h"
#include "arm9/boot_cfg.h"
#include "arm9/console.h"
#include "arm9/installer_ui.h"
#include "arm9/ipc.h"
#include "arm9/loader.h"
#include "arm9/mpu.h"
#include "arm9/nand_boot.h"
#include "arm9/scfg.h"
#include "arm9/storage.h"
#include "arm9/theme.h"
#include "arm9/ui_font.h"
#include "arm9/video.h"
#include "common/dsi_hw.h"
#include "common/fat_ro.h"
#include "common/ipc_protocol.h"
#include "common/lz77.h"
#include "common/mem.h"
#include "common/nds_header.h"

/* Poll budget for one IPC round-trip (~tens of ms at 134 MHz). */
#define MENU_IPC_TIMEOUT   0x00200000u

/* ---- Menu content ----------------------------------------------------- */

static const char *const s_menu_labels[MENU_ITEM_COUNT] = {
    "[1] Re-run IPC & HW Test",
    "[2] Inspect SCFG / MBK",
    "[3] Probe SD / TMIO RO",
    "[4] LZ77 Decompress Verify",
    "[5] Reboot / Halt System",
    "[6] Browse SD Card (.nds)",
    "[7] Init SCFG / MBK",
    "[8] Autoboot BOOT.CFG",
    "[9] Inspect BOOT.CFG",
    "[10] Install to NAND",
};

static const char *const s_menu_desc[MENU_ITEM_COUNT] = {
    "IPC + RTC + SD + FW self-test",
    "Dump SCFG_EXT / SCFG_CLK / MBK",
    "TMIO SD read-only status probe",
    "Expand embedded LZ77 vector",
    "Halt the CPU safely",
    "Mount FAT, browse root + /NDS/",
    "Unlock + map DSi SCFG/NWRAM",
    "Boot the resolved BOOT.CFG target",
    "BOOT.CFG source + hotkey map",
    "Backup NAND then flash (guarded)",
};

/* Embedded LZ77 self-test vector: literals "ABCD" + length-4/distance-4 back
 * reference => "ABCDABCD" (matches tests/test_arm9.c). */
static const u8 s_lz_stream[] = {
    NDS_LZ77_TYPE10, 0x08u, 0x00u, 0x00u,
    0x08u, 'A', 'B', 'C', 'D', 0x10u, 0x03u,
};
static const u8 s_lz_expected[] = "ABCDABCD";

/*
 * Destination for the delegated firmware read. It lives in the ARM9 image's
 * .bss, which is in shared Main RAM (0x0220xxxx) and therefore visible to the
 * ARM7. Never place this in DTCM: the ARM7 cannot reach DTCM.
 */
static u8 s_fw_probe[8];

/*
 * SD browser state. All of it lives in .bss (shared Main RAM) because the
 * FAT volume's sector buffer is filled directly by the ARM7 over IPC.
 */
static fat_volume_t   s_fat_vol;
static fat_volume_t   s_nand_vol;
static fat_file_entry_t s_fat_files[MENU_BROWSE_MAX];
static char s_fat_paths[MENU_BROWSE_MAX][BOOT_CFG_PATH_MAX];

/* Unlaunch-style autoboot target resolved from BOOT.CFG + the SD directory. */
static fat_file_entry_t s_autoboot_entry;
static char s_autoboot_path[BOOT_CFG_PATH_MAX];
static bool s_autoboot_entry_valid;

/* Parsed BOOT.CFG (or the built-in defaults) and the startup decision. */
static boot_cfg_t   s_boot_cfg;
static bool         s_boot_cfg_present;
static boot_target_t s_boot_target;

/* Short theme-status line shown on both screens (set by menu_load_theme). */
static char s_theme_footer[24];

/* The optional one-level subdirectory scanned by the browser (see §28). */
#define MENU_SUBDIR_NAME "NDS"
#define MENU_SUBDIR_PREFIX "NDS/"

/* ---- Pure menu state machine ----------------------------------------- */

void menu_state_init(menu_state_t *m, u8 count)
{
    if (m == NULL) {
        return;
    }
    m->cursor = 0u;
    m->count = count;
}

void menu_state_move(menu_state_t *m, s32 delta)
{
    s32 v;

    if (m == NULL || m->count == 0u) {
        return;
    }

    v = (s32)m->cursor + delta;
    while (v < 0) {
        v += (s32)m->count;
    }
    while (v >= (s32)m->count) {
        v -= (s32)m->count;
    }
    m->cursor = (u8)v;
}

u8 menu_state_selected(const menu_state_t *m)
{
    return (m != NULL) ? m->cursor : 0u;
}

menu_action_t menu_action_for(const menu_state_t *m)
{
    if (m == NULL) {
        return MENU_ACTION_NONE;
    }
    switch (m->cursor) {
    case 0u: return MENU_ACTION_SELFTEST;
    case 1u: return MENU_ACTION_INSPECT;
    case 2u: return MENU_ACTION_SD;
    case 3u: return MENU_ACTION_LZ77;
    case 4u: return MENU_ACTION_HALT;
    case 5u: return MENU_ACTION_BROWSE;
    case 6u: return MENU_ACTION_SCFG_INIT;
    case 7u: return MENU_ACTION_AUTOBOOT;
    case 8u: return MENU_ACTION_INSPECT_CFG;
    case 9u: return MENU_ACTION_INSTALL;
    default: return MENU_ACTION_NONE;
    }
}

/* ---- Diagnostics ------------------------------------------------------ */

static void diag_ping(menu_diag_t *d)
{
    u32 reply;
    u32 i;

    d->ipc_ok = false;

    for (i = 0u; i < MENU_IPC_TIMEOUT; i++) {
        if ((i & 0x3FFFu) == 0u) {
            (void)arm9_ipc_try_send(IPC_CH_SYSTEM, IPC_CMD_PING);
        }
        if (arm9_ipc_try_recv(&reply)) {
            if (ipc_channel(reply) == IPC_CH_SYSTEM &&
                (ipc_payload(reply) & 0xFFFFu) == IPC_CMD_PONG) {
                d->ipc_ok = true;
                d->ipc_ping_count++;
                return;
            }
        }
    }
}

static void diag_query_rtc(menu_diag_t *d)
{
    u32 r[4];

    d->rtc_valid = false;
    if (arm9_ipc_query(IPC_CH_SYSTEM, IPC_CMD_GET_RTC, r, 4u, MENU_IPC_TIMEOUT) >= 4u) {
        if ((ipc_reply_status(r[0]) & IPC_RTC_STATUS_VALID) != 0u) {
            d->year = ipc_unpack_triple_hi(r[1]);
            d->month = ipc_unpack_triple_mid(r[1]);
            d->day = ipc_unpack_triple_lo(r[1]);
            d->hour = ipc_unpack_triple_hi(r[2]);
            d->minute = ipc_unpack_triple_mid(r[2]);
            d->second = ipc_unpack_triple_lo(r[2]);
            d->rtc_valid = true;
        }
    }
}

static void diag_query_sd(menu_diag_t *d)
{
    u32 r[4];

    d->sd_flags = IPC_SD_FLAG_UNAVAILABLE;
    d->sd_raw = 0u;
    d->sd_card_type = 0u;

    if (arm9_ipc_query(IPC_CH_SYSTEM, IPC_CMD_GET_SDMMC_STATUS, r, 4u,
                       MENU_IPC_TIMEOUT) >= 4u) {
        d->sd_flags = (u8)r[1];
        d->sd_raw = r[2];
        d->sd_card_type = (r[3] & IPC_SD_INFO_BLOCK_ADDRESSED) ? 1u : 0u;
    }
}

/*
 * Delegated SD read smoke test: read LBA 0 (the MBR) through the ARM7 and check
 * the boot signature. This is the end-to-end proof of the storage pipeline.
 */
static void diag_sd_read(menu_diag_t *d)
{
    arm9_storage_probe_t probe;

    arm9_storage_probe(&probe);
    d->sd_read_ok = probe.ok;
    d->sd_mbr_ok = probe.mbr_signature;
    d->sd_fs_type = probe.fs_type;
    d->sd_total_sectors = probe.total_sectors;
}

static void diag_query_fw(menu_diag_t *d)
{
    u32 r[3];

    d->fw_ok = false;
    d->fw_id = 0u;

    if (arm9_ipc_query(IPC_CH_SYSTEM, IPC_CMD_GET_FW_INFO, r, 3u,
                       MENU_IPC_TIMEOUT) >= 3u) {
        d->fw_ok = (r[1] & IPC_FW_FLAG_OK) != 0u;
        d->fw_id = r[2];
    }
}

/*
 * Delegated firmware read: the ARM9 does not touch SPI. It asks the ARM7 (over
 * IPC) to clock the bytes into s_fw_probe and reports the first word. This
 * exercises the ARM7-gated path end to end.
 */
static void diag_read_firmware(menu_diag_t *d)
{
    d->fw_read_ok = false;
    d->fw_read_word = 0u;

    if (arm9_firmware_read(0u, s_fw_probe, sizeof(s_fw_probe))) {
        d->fw_read_word = ((u32)s_fw_probe[0] << 24) |
                          ((u32)s_fw_probe[1] << 16) |
                          ((u32)s_fw_probe[2] << 8) |
                          (u32)s_fw_probe[3];
        d->fw_read_ok = true;
    }
}

static void diag_lz77(menu_diag_t *d)
{
    u8 out[16];
    u32 n;

    dsi_memset(out, 0, sizeof(out));
    n = nds_lz77_decompress(s_lz_stream, (u32)sizeof(s_lz_stream),
                            out, (u32)sizeof(out));

    d->lz77_bytes = n;
    d->lz77_ok = (n == 8u) &&
                 (dsi_memcmp(out, s_lz_expected, 8u) == 0);
}

static void diag_poll_extkey(void)
{
    u32 r[2];
    u16 ext = 0u;

    if (arm9_ipc_query(IPC_CH_SYSTEM, IPC_CMD_GET_EXTKEY, r, 2u,
                       MENU_IPC_TIMEOUT) >= 2u) {
        u32 keys = r[1];

        if ((keys & IPC_EXTKEY_X) != 0u) {
            ext |= INPUT_X;
        }
        if ((keys & IPC_EXTKEY_Y) != 0u) {
            ext |= INPUT_Y;
        }
        if ((keys & IPC_EXTKEY_PEN) != 0u) {
            ext |= INPUT_PEN;
        }
        if ((keys & IPC_EXTKEY_HINGE) != 0u) {
            ext |= INPUT_HINGE;
        }
    }
    input_update_ext(ext);
}

/*
 * Read the internal NAND's LBA-0 partition table through the ARM7 and record
 * the result. Read-only: no NAND command is ever issued (the project conventions).
 */
static void diag_nand(menu_diag_t *d)
{
    arm9_nand_probe_t probe;

    arm9_nand_probe(&probe);
    d->nand_ok = probe.ok && probe.signature_ok;
    d->nand_fat = probe.fat_present;
    d->nand_format = probe.format;
    d->nand_partitions = probe.partitions;
    d->nand_fat_lba = probe.fat_lba;
    d->nand_total_sectors = probe.total_sectors;
}

static void diag_collect(menu_diag_t *d)
{
    scfg_state_t scfg;
    u32 i;

    dsi_memset(d, 0, sizeof(*d));

    scfg_read_state(&scfg);
    d->scfg_accessible = scfg.accessible;
    d->twl_mode        = scfg.twl_mode;
    d->extended_ram    = scfg.extended_ram;
    d->ndma            = scfg.ndma;
    d->scfg_unlocked   = scfg.scfg_unlocked;
    d->a9rom           = scfg.a9rom;
    d->scfg_clk        = scfg.clk;
    d->scfg_ext        = scfg.ext;
    d->arm9_hz         = scfg.arm9_hz;
    for (i = 0u; i < MBK_COUNT; i++) {
        d->mbk[i] = scfg.mbk[i];
    }

    d->mpu_control = arm9_mpu_read_control();
    d->dtcm_base = DTCM_BASE;

    diag_ping(d);
    diag_query_rtc(d);
    diag_query_sd(d);
    diag_sd_read(d);
    diag_query_fw(d);
    diag_read_firmware(d);
    diag_lz77(d);
    diag_nand(d);
}

/* ---- Unlaunch-style BOOT.CFG / target autoboot ----------------------- */

/* Copy a C string into a fixed buffer, always NUL-terminating. */
static void menu_copy_str(char *dst, u32 cap, const char *src)
{
    u32 i = 0u;

    if (dst == NULL || cap == 0u) {
        return;
    }
    if (src != NULL) {
        while (src[i] != '\0' && i + 1u < cap) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

/* Build "<prefix><name>" into `dst` (a NULL prefix is treated as empty). */
static void menu_join_path(char *dst, u32 cap, const char *prefix,
                           const char *name)
{
    u32 n = 0u;

    if (dst == NULL || cap == 0u) {
        return;
    }
    if (prefix != NULL) {
        u32 i = 0u;

        while (prefix[i] != '\0' && n + 1u < cap) {
            dst[n++] = prefix[i];
            i++;
        }
    }
    if (name != NULL) {
        u32 i = 0u;

        while (name[i] != '\0' && n + 1u < cap) {
            dst[n++] = name[i];
            i++;
        }
    }
    dst[n] = '\0';
}

/* BOOT.CFG read buffer; lives in .bss (shared Main RAM), never DTCM. */
static u8 s_boot_cfg_buf[BOOT_CFG_READ_LEN];

/*
 * Read up to one sector of BOOT.CFG from the SD root (if present) and parse it.
 * On any failure the built-in defaults remain in effect. Results are cached in
 * s_boot_cfg / s_boot_cfg_present.
 */
static void menu_load_boot_cfg(menu_diag_t *d)
{
    fat_file_entry_t cfg;
    u32 got = 0u;
    fat_chain_status_t chain = FAT_CHAIN_ERR_ARG;

    boot_cfg_defaults(&s_boot_cfg);
    s_boot_cfg_present = false;
    d->boot_cfg_present = false;

    diag_query_sd(d);
    if ((d->sd_flags & IPC_SD_FLAG_UNAVAILABLE) != 0u ||
        (d->sd_flags & IPC_SD_FLAG_PRESENT) == 0u) {
        return;
    }
    if (!arm9_storage_mount(&s_fat_vol, IPC_STORAGE_DEV_SD)) {
        return;
    }
    if (!fat_find_path(&s_fat_vol, "BOOT.CFG", &cfg) || cfg.is_dir) {
        return;
    }

    /*
     * A short file is read in full (fat_read_file_range() clamps to the file
     * size); a longer one yields the first sector. A partial read still leaves
     * a usable prefix in the buffer.
     */
    (void)fat_read_file_range(&s_fat_vol, &cfg, 0u, s_boot_cfg_buf,
                              sizeof(s_boot_cfg_buf), &got, &chain);
    if (got == 0u) {
        return;
    }

    (void)boot_cfg_parse(s_boot_cfg_buf, got, &s_boot_cfg);
    s_boot_cfg_present = true;
    d->boot_cfg_present = true;
}

/*
 * Apply the parsed BOOT.CFG theme (or the AMOLED defaults) to both consoles and
 * attempt to stage + present the optional SPLASH.RAW backdrop. The splash is
 * streamed through the delegated FAT reader into a static staging buffer and
 * blitted to Engine A BG2; a missing or invalid image falls back cleanly to the
 * accent-coloured UI over the pure THEME_BG backdrop.
 */
static void menu_load_theme(menu_diag_t *d)
{
    theme_splash_status_t st = THEME_SPLASH_NONE;
    u32 got = 0u;

    d->theme_applied = true;
    d->splash_loaded = false;
    d->splash_status = (u8)THEME_SPLASH_NONE;

    theme_apply(&s_boot_cfg.theme);

    if (!s_boot_cfg.theme.has_splash) {
        /*
         * No SPLASH_IMAGE configured: present the built-in OpenLaunch penguin
         * splash. It needs no card, so the boot screen is never blank.
         */
        st = theme_load_splash(NULL, &s_boot_cfg.theme, &got);
        d->splash_status = (u8)st;
        if (st == THEME_SPLASH_BUILTIN && theme_present_splash()) {
            d->splash_loaded = true;
            menu_copy_str(s_theme_footer, sizeof(s_theme_footer), "THEME: TUX");
        } else {
            menu_copy_str(s_theme_footer, sizeof(s_theme_footer),
                          s_boot_cfg_present ? "THEME: BOOT.CFG" : "THEME: DEFAULT");
        }
        return;
    }

    diag_query_sd(d);
    if ((d->sd_flags & IPC_SD_FLAG_UNAVAILABLE) == 0u &&
        (d->sd_flags & IPC_SD_FLAG_PRESENT) != 0u) {
        if (arm9_storage_mount(&s_fat_vol, IPC_STORAGE_DEV_SD)) {
            st = theme_load_splash(&s_fat_vol, &s_boot_cfg.theme, &got);
        } else {
            st = THEME_SPLASH_ERR_IO;
        }
    }

    d->splash_status = (u8)st;
    if (st == THEME_SPLASH_OK && theme_present_splash()) {
        d->splash_loaded = true;
    }

    if (d->splash_loaded) {
        menu_copy_str(s_theme_footer, sizeof(s_theme_footer), "THEME: SPLASH OK");
    } else if (st == THEME_SPLASH_OK) {
        menu_copy_str(s_theme_footer, sizeof(s_theme_footer), "THEME: SPLASH RAM");
    } else {
        menu_copy_str(s_theme_footer, sizeof(s_theme_footer), "THEME: DEFAULT");
    }
}

/*
 * Resolve the startup target and, when autobooting, locate it on the mounted SD
 * volume. The pure policy decision is `boot_resolve_target()`; only the file
 * discovery touches the card.
 */
static void menu_probe_autoboot(menu_diag_t *d, const boot_target_t *target)
{
    const char *picked = NULL;

    d->autoboot_checked = false;
    d->autoboot_ready = false;
    d->autoboot_key_held = false;
    d->autoboot_name[0] = '\0';
    d->autoboot_sel[0] = '\0';
    s_autoboot_entry_valid = false;
    s_autoboot_path[0] = '\0';

    if (target != NULL && target->policy == BOOT_POLICY_MENU) {
        d->autoboot_key_held = true;
        d->last_action = "MENU (KEY HELD)";
        return;
    }

    diag_query_sd(d);
    if ((d->sd_flags & IPC_SD_FLAG_UNAVAILABLE) != 0u ||
        (d->sd_flags & IPC_SD_FLAG_PRESENT) == 0u) {
        d->autoboot_checked = true;
        return;
    }
    if (!arm9_storage_mount(&s_fat_vol, IPC_STORAGE_DEV_SD)) {
        d->autoboot_checked = true;
        return;
    }

    /*
     * Prefer the resolved (possibly subdirectory) target; fall back to the
     * classic HELLO.NDS so the deterministic test image still autoboots.
     */
    if (target != NULL && target->target != NULL &&
        fat_find_path(&s_fat_vol, target->target, &s_autoboot_entry) &&
        !s_autoboot_entry.is_dir) {
        picked = target->target;
    } else if (fat_find_path(&s_fat_vol, BOOT_FALLBACK_NDS_NAME,
                             &s_autoboot_entry) &&
               !s_autoboot_entry.is_dir) {
        picked = BOOT_FALLBACK_NDS_NAME;
    }

    if (picked != NULL) {
        s_autoboot_entry_valid = true;
        d->autoboot_ready = true;
        menu_copy_str(s_autoboot_path, sizeof(s_autoboot_path), picked);
        menu_copy_str(d->autoboot_name, sizeof(d->autoboot_name), picked);

        if (target != NULL && picked == target->target) {
            if (target->has_key && target->label != NULL) {
                menu_copy_str(d->autoboot_sel, sizeof(d->autoboot_sel),
                              target->label);
            } else if (s_boot_cfg_present) {
                menu_copy_str(d->autoboot_sel, sizeof(d->autoboot_sel), "CFG");
            }
        }
        d->last_action = "Autoboot target resolved";
    }
    d->autoboot_checked = true;
}

/* ---- Rendering -------------------------------------------------------- */

static const char *sd_status_text(u8 flags)
{
    if ((flags & IPC_SD_FLAG_UNAVAILABLE) != 0u) {
        return "N/A (DS mode)";
    }
    if ((flags & IPC_SD_FLAG_ERROR) != 0u) {
        return "ERROR";
    }
    if ((flags & IPC_SD_FLAG_PRESENT) != 0u) {
        return ((flags & IPC_SD_FLAG_READONLY) != 0u) ? "PRESENT RO" : "PRESENT RW";
    }
    return "NO CARD";
}

static void wait_vblank(void)
{
    while ((REG16(DISPSTAT) & 0x0001u) == 0u) {
        /* wait for VBlank */
    }
    while ((REG16(DISPSTAT) & 0x0001u) != 0u) {
        /* wait for the end of VBlank */
    }
}

static void render_telemetry(const menu_diag_t *d)
{
    console_select(CONSOLE_MAIN);
    console_clear();

    console_puts(0, 0, CONSOLE_PAL_CYAN, "OPENLAUNCH TWL BOOTLOADER");
    if (s_theme_footer[0] != '\0') {
        console_puts(0, 1, CONSOLE_PAL_CYAN, s_theme_footer);
    } else {
        console_puts(0, 1, CONSOLE_PAL_GREY, "CLEAN-ROOM TWL DIAGNOSTICS");
    }

    if (d->scfg_accessible) {
        console_printf(0, 2, CONSOLE_PAL_GREEN, "SCFG : %s %uMB%s",
                       d->twl_mode ? "DSi/TWL" : "DSi NDS-mode",
                       (unsigned)(scfg_ext_ram_bytes(d->scfg_ext) >> 20),
                       d->scfg_unlocked ? " UNLOCKED" : " locked");
    } else {
        console_puts(0, 2, CONSOLE_PAL_GREY, "SCFG : N/A (DS mode)");
    }

    console_printf(0, 3, CONSOLE_PAL_WHITE, "CPU  : %s %uMHz",
                   d->twl_mode ? "DSi/TWL" : "DS legacy",
                   (unsigned)(d->arm9_hz / 1000000u));
    console_printf(0, 4, CONSOLE_PAL_WHITE, "DTCM : %08X", d->dtcm_base);
    console_printf(0, 5, CONSOLE_PAL_WHITE, "MPU  : %08X", d->mpu_control);

    console_printf(0, 6, d->ipc_ok ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                   "ARM7 : %s (%u)", d->ipc_ok ? "OK" : "TIMEOUT",
                   (unsigned)d->ipc_ping_count);

    {
        u8 sd_pal = ((d->sd_flags & (IPC_SD_FLAG_ERROR | IPC_SD_FLAG_UNAVAILABLE)) != 0u)
                        ? CONSOLE_PAL_YELLOW : CONSOLE_PAL_GREEN;

        if ((d->sd_flags & IPC_SD_FLAG_PRESENT) != 0u) {
            console_printf(0, 7, sd_pal, "SD   : %s %s",
                           sd_status_text(d->sd_flags),
                           (d->sd_card_type != 0u) ? "SDHC" : "SDSC");
        } else {
            console_printf(0, 7, sd_pal, "SD   : %s",
                           sd_status_text(d->sd_flags));
        }
    }

    if (d->rtc_valid) {
        console_printf(0, 8, CONSOLE_PAL_GREEN, "RTC  : %02u-%02u-%02u %02u:%02u:%02u",
                       (unsigned)d->year, (unsigned)d->month, (unsigned)d->day,
                       (unsigned)d->hour, (unsigned)d->minute, (unsigned)d->second);
    } else {
        console_puts(0, 8, CONSOLE_PAL_YELLOW, "RTC  : unavailable");
    }

    if (d->fw_ok) {
        console_printf(0, 9, CONSOLE_PAL_GREEN, "FW   : ID %06X", d->fw_id);
    } else {
        console_puts(0, 9, CONSOLE_PAL_GREY, "FW   : not detected");
    }

#ifdef ENABLE_NAND_WRITE_DANGER
    console_puts(0, 10, CONSOLE_PAL_RED, "GUARD: NAND WRITE ARMED!");
#else
    console_puts(0, 10, CONSOLE_PAL_GREEN, "GUARD: NAND WRITE LOCKED");
#endif

    console_printf(0, 11, d->lz77_ok ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                   "LZ77 : %s (%u bytes)", d->lz77_ok ? "OK" : "FAIL",
                   (unsigned)d->lz77_bytes);

    if (d->fw_read_ok) {
        console_printf(0, 12, CONSOLE_PAL_GREEN, "FW RD: %08X (ARM7 IPC)",
                       d->fw_read_word);
    } else {
        console_puts(0, 12, CONSOLE_PAL_YELLOW, "FW RD: unavailable");
    }

    console_printf(0, 13, CONSOLE_PAL_GREY, "LAST : %s",
                   (d->last_action != NULL) ? d->last_action : "-");

    if (d->sd_read_ok) {
        const char *fs = "none";

        if (d->sd_fs_type == FAT_TYPE_FAT16) {
            fs = "FAT16";
        } else if (d->sd_fs_type == FAT_TYPE_FAT32) {
            fs = "FAT32";
        }
        console_printf(0, 14, CONSOLE_PAL_GREEN, "SD RD: OK MBR:%s FS:%s",
                       d->sd_mbr_ok ? "YES" : "NO", fs);
    } else {
        console_puts(0, 14, CONSOLE_PAL_YELLOW, "SD RD: unavailable");
    }

    /*
     * Row 16 is shared: an in-progress staging report takes precedence over the
     * static card-capacity readout.
     */
    if (d->boot_staged) {
        console_printf(0, 16,
                       d->boot_dry_run ? CONSOLE_PAL_YELLOW : CONSOLE_PAL_GREEN,
                       "STAGE: %s",
                       d->boot_dry_run ? "DRY-RUN OK" : "BOOTING...");
    } else if (d->loader_inspected) {
        /* Row 16 also carries the decoded banner in the verify/dry-run view. */
        if (d->banner_present && d->banner_crc_ok) {
            console_printf(0, 16, CONSOLE_PAL_GREEN, "BANNER: %s",
                           d->banner_title);
        } else if (d->banner_present) {
            console_puts(0, 16, CONSOLE_PAL_RED, "BANNER: BAD CRC");
        } else {
            console_puts(0, 16, CONSOLE_PAL_GREY, "BANNER: NONE");
        }
    } else if (d->nand_ok) {
        console_puts(0, 16, CONSOLE_PAL_GREEN, "NAND : DETECTED");
    } else {
        console_puts(0, 16, CONSOLE_PAL_GREY, "NAND : ABSENT");
    }

    /* Row 17 carries the SD capacity in the normal view (free otherwise). */
    if (!d->loader_inspected && d->sd_total_sectors != 0u) {
        console_printf(0, 17, CONSOLE_PAL_GREY, "SDCAP: %u MiB",
                       (unsigned)((d->sd_total_sectors + 2047u) / 2048u));
    }

    if (d->autoboot_key_held) {
        console_puts(0, 15, CONSOLE_PAL_YELLOW, "BOOT : MENU (KEY HELD)");
    } else if (d->autoboot_checked) {
        if (d->autoboot_ready) {
            if (d->autoboot_sel[0] != '\0') {
                console_printf(0, 15, CONSOLE_PAL_GREEN, "BOOT : %s [%s]",
                               d->autoboot_name, d->autoboot_sel);
            } else {
                console_printf(0, 15, CONSOLE_PAL_GREEN, "BOOT : %s",
                               d->autoboot_name);
            }
        } else {
            console_puts(0, 15, CONSOLE_PAL_YELLOW, "BOOT : NO BOOT.NDS");
        }
    } else if (d->boot_loaded) {
        console_printf(0, 15,
                       d->boot_crc_ok ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                       "BOOT : %s %s %s", d->boot_title, d->boot_code,
                       d->boot_crc_ok ? "CRC-OK" : "CRC-BAD");
    } else {
        console_puts(0, 15, CONSOLE_PAL_GREY, "BOOT : -");
    }

    /* Chainloader verify / dry-run detail (item [6], Y key). */
    if (d->loader_inspected) {
        console_printf(0, 17, CONSOLE_PAL_CYAN, "TITLE: %s %s U%02X",
                       d->boot_title, d->boot_code,
                       (unsigned)d->loader_unit_code);
        console_printf(0, 18, CONSOLE_PAL_WHITE, "A9 E:%08X",
                       d->loader_arm9_entry);
        console_printf(0, 19, CONSOLE_PAL_WHITE, "A9 L:%08X S:%06X",
                       d->loader_arm9_load, (unsigned)d->loader_arm9_size);
        console_printf(0, 20, CONSOLE_PAL_WHITE, "A7 E:%08X",
                       d->loader_arm7_entry);
        console_printf(0, 21, CONSOLE_PAL_WHITE, "A7 L:%08X S:%06X",
                       d->loader_arm7_load, (unsigned)d->loader_arm7_size);
        console_printf(0, 22,
                       d->loader_loadable ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                       "LOADABLE: %s",
                       d->loader_loadable ? "YES" : nds_load_status_text(
                           (nds_load_status_t)d->loader_status));

        if (d->dldi_present) {
            console_printf(0, 23, CONSOLE_PAL_CYAN, "DLDI: YES (%s, %uK)",
                           d->dldi_io_type, (unsigned)d->dldi_alloc_kb);
        } else {
            console_puts(0, 23, CONSOLE_PAL_GREY, "DLDI: NONE");
        }
    }
}

static void render_menu(const menu_state_t *state)
{
    u8 i;

    console_select(CONSOLE_SUB);
    console_clear();

    console_puts(0, 0, CONSOLE_PAL_CYAN, "MAIN MENU");

    for (i = 0u; i < state->count && i < MENU_ITEM_COUNT; i++) {
        char line[CONSOLE_COLS + 1];
        u8 pal = (i == state->cursor) ? CONSOLE_PAL_YELLOW : CONSOLE_PAL_WHITE;

        console_format(line, sizeof(line), "%c%s",
                       (i == state->cursor) ? '>' : ' ', s_menu_labels[i]);
        console_puts(0, 2 + (int)i, pal, line);
    }

    console_puts(0, 12, CONSOLE_PAL_GREY, "UP/DOWN SELECT  A RUN");
    console_puts(0, 13, CONSOLE_PAL_GREY, "B: RE-RUN SELF-TEST");
    console_puts(0, 15, CONSOLE_PAL_CYAN, "INFO:");
    console_puts(0, 16, CONSOLE_PAL_WHITE, s_menu_desc[state->cursor]);
    if (s_theme_footer[0] != '\0') {
        console_puts(0, 18, CONSOLE_PAL_CYAN, s_theme_footer);
    }
}

/* ---- Actions ---------------------------------------------------------- */

static void menu_halt(void) __attribute__((noreturn));

static void menu_halt(void)
{
    console_select(CONSOLE_MAIN);
    console_printf(0, 22, CONSOLE_PAL_RED, "SYSTEM HALTED (SAFE)");
    console_select(CONSOLE_SUB);
    console_printf(0, 22, CONSOLE_PAL_RED, "SYSTEM HALTED (SAFE)");

    /*
     * HALTCNT (0x04000301) is the ARM7 power-down register; the ARM9 halts
     * through BIOS SWI 0x02 instead. No NAND/eMMC access is performed.
     */
    for (;;) {
        dsi_halt();
    }
}

static void menu_inspect_view(const menu_diag_t *d)
{
    input_state_t keys = { 0u, 0u, 0u };
    u32 i;

    console_select(CONSOLE_MAIN);
    console_clear();

    console_puts(0, 0, CONSOLE_PAL_CYAN, "REGISTER INSPECTION");

    if (d->scfg_accessible) {
        console_printf(0, 2, CONSOLE_PAL_GREEN, "MODE  : %s",
                       d->twl_mode ? "DSi/TWL mode" : "DSi (NDS compat)");
    } else {
        console_puts(0, 2, CONSOLE_PAL_GREY, "MODE  : legacy DS (no SCFG)");
    }
    console_printf(0, 3, CONSOLE_PAL_WHITE, "A9ROM : %08X", d->a9rom);
    console_printf(0, 4, CONSOLE_PAL_WHITE, "CLK   : %08X (%u MHz)",
                   d->scfg_clk, (unsigned)(d->arm9_hz / 1000000u));
    console_printf(0, 5, CONSOLE_PAL_WHITE, "EXT   : %08X RAM-%uMB%s",
                   d->scfg_ext,
                   (unsigned)(scfg_ext_ram_bytes(d->scfg_ext) >> 20),
                   d->scfg_unlocked ? " UL" : " LK");
    console_printf(0, 6, CONSOLE_PAL_WHITE, "MPU   : %08X", d->mpu_control);
    console_printf(0, 7, CONSOLE_PAL_WHITE, "DTCM  : %08X", d->dtcm_base);

    if (d->scfg_accessible) {
        for (i = 0u; i < MBK_COUNT; i++) {
            console_printf(0, 9 + (int)i, CONSOLE_PAL_GREY, "MBK%u : %08X",
                           (unsigned)(i + 1u), d->mbk[i]);
        }
        console_puts(0, 19, CONSOLE_PAL_GREY, "NWRAM A->ARM7  B/C->ARM9");
    } else {
        console_puts(0, 9, CONSOLE_PAL_GREY, "MBK1-9: N/A (DS mode)");
    }

    console_puts(0, 21, CONSOLE_PAL_YELLOW, "PRESS B TO RETURN");

    for (;;) {
        diag_poll_extkey();
        input_poll(&keys);
        if (input_pressed(&keys, INPUT_B)) {
            break;
        }
        wait_vblank();
    }
}

/*
 * Format a BOOT.CFG key label padded to the 7-character display column
 * ("DEFAULT", "A      ", ...). `dst` always ends NUL-terminated.
 */
static void menu_bootcfg_label(char *dst, u32 cap, boot_cfg_key_t key)
{
    const char *name = boot_cfg_key_name(key);
    u32 i = 0u;

    if (dst == NULL || cap == 0u) {
        return;
    }
    while (name[i] != '\0' && i + 1u < cap) {
        dst[i] = name[i];
        i++;
    }
    /* Pad the key column to 7 characters ("DEFAULT" / "A      "). */
    while (i + 1u < cap && i < 7u) {
        dst[i] = ' ';
        i++;
    }
    dst[i] = '\0';
}

/*
 * Menu item [9]: inspect the parsed BOOT.CFG / hotkey map. It reports where the
 * configuration came from (the SD root or the built-in defaults), every
 * recognised binding with a live SD target check (OK / MISSING), and the
 * unbrickable B/SELECT override. Read-only: it mounts, resolves and renders.
 */
static void menu_bootcfg_view(menu_diag_t *d)
{
    input_state_t keys = { 0u, 0u, 0u };
    bool mounted = false;
    u32 i;

    d->last_action = "Inspect BOOT.CFG";

    diag_query_sd(d);
    if ((d->sd_flags & IPC_SD_FLAG_UNAVAILABLE) == 0u &&
        (d->sd_flags & IPC_SD_FLAG_PRESENT) != 0u) {
        mounted = arm9_storage_mount(&s_fat_vol, IPC_STORAGE_DEV_SD);
    }

    console_select(CONSOLE_MAIN);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_CYAN, "BOOT.CFG / HOTKEY INSPECT");

    console_printf(0, 2,
                   s_boot_cfg_present ? CONSOLE_PAL_GREEN : CONSOLE_PAL_YELLOW,
                   "SOURCE : %s",
                   s_boot_cfg_present ? "SD:/BOOT.CFG" : "DEFAULTS");
    console_printf(0, 3, mounted ? CONSOLE_PAL_GREEN : CONSOLE_PAL_GREY,
                   "SD     : %s", mounted ? "MOUNTED" : "UNAVAILABLE");

    console_puts(0, 5, CONSOLE_PAL_WHITE, "BINDING / TARGET");

    for (i = 0u; i < BOOT_CFG_KEY_COUNT; i++) {
        char label[8];
        const char *t = s_boot_cfg.target[i];

        menu_bootcfg_label(label, sizeof(label), (boot_cfg_key_t)i);

        if (t[0] == '\0') {
            console_printf(0, 6 + (int)i, CONSOLE_PAL_GREY, "%s: -", label);
        } else {
            bool ok = false;

            if (mounted) {
                fat_file_entry_t e;

                ok = fat_find_path(&s_fat_vol, t, &e) && !e.is_dir;
            }
            console_printf(0, 6 + (int)i,
                           ok ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                           "%s: %s [%s]", label, t, ok ? "OK" : "MISSING");
        }
    }

    console_puts(0, 14, CONSOLE_PAL_YELLOW, "B/SEL  : FORCE MENU (LOCKED)");
    console_puts(0, 16, CONSOLE_PAL_GREY, "B: RETURN");

    /* Seed the edge detector so the A press that opened this view is ignored. */
    input_poll(&keys);

    for (;;) {
        wait_vblank();
        diag_poll_extkey();
        input_poll(&keys);
        if (input_pressed(&keys, INPUT_B)) {
            break;
        }
    }
}

/*
 * Verify / dry-run inspect: read the selected file's 512-byte header through
 * the delegated FAT reader, run the full loadability validation (CRC16 plus
 * ARM9/ARM7 range safety) and decode the descriptors for the top screen.
 */
static void loader_inspect(menu_diag_t *d, fat_volume_t *vol,
                           const fat_file_entry_t *entry)
{
    nds_header_t h;
    nds_load_status_t st;

    d->loader_inspected = false;
    d->loader_loadable = false;
    d->boot_loaded = false;
    d->boot_staged = false;
    d->boot_dry_run = false;
    d->dldi_present = false;
    d->dldi_patched = false;
    d->dldi_alloc_kb = 0u;
    d->dldi_io_type[0] = '\0';
    d->banner_present = false;
    d->banner_crc_ok = false;
    d->banner_title[0] = '\0';

    st = loader_validate_file(vol, entry, d->extended_ram, &h);

    d->loader_status = (u8)st;
    d->loader_loadable = (st == NDS_LOAD_OK);

    /* loader_validate_file() zero-fills `h`, so this is always safe. */
    dsi_memcpy(d->boot_title, h.title, sizeof(h.title));
    dsi_memcpy(d->boot_code, h.gamecode, sizeof(h.gamecode));
    d->boot_crc_ok = h.crc_ok;
    d->boot_loaded = true;

    d->loader_unit_code = h.unit_code;
    d->loader_arm9_entry = h.arm9_entry;
    d->loader_arm9_load  = h.arm9_ram_addr;
    d->loader_arm9_size  = h.arm9_size;
    d->loader_arm7_entry = h.arm7_entry;
    d->loader_arm7_load  = h.arm7_ram_addr;
    d->loader_arm7_size  = h.arm7_size;

    /*
     * Bounded DLDI scan of the ARM9 segment prefix. This is a read-only
     * inspection: patching only happens on the staging path (item [6] A).
     */
    if (h.arm9_size != 0u && h.arm9_rom_offset != 0u) {
        dldi_info_t dinfo;

        if (loader_detect_dldi(vol, entry, &h, &dinfo)) {
            d->dldi_present = true;
            d->dldi_alloc_kb = dinfo.allocated_size >> 10;
            dsi_memcpy(d->dldi_io_type, dinfo.io_type,
                       sizeof(d->dldi_io_type));
        }
    }

    /*
     * Read and decode the optional 0x840-byte banner. A banner is only
     * accepted when its version is recognised; a bad CRC16 is reported
     * separately. Entirely read-only.
     */
    {
        nds_banner_info_t binfo;

        if (loader_read_banner(vol, entry, &h, &binfo) && binfo.version_ok) {
            d->banner_present = true;
            d->banner_crc_ok = binfo.crc_ok;
            dsi_memcpy(d->banner_title, binfo.title, sizeof(d->banner_title));
        }
    }

    d->loader_inspected  = true;
}

/*
 * Stage the selected image and prepare the dual-CPU handoff. A verified image
 * that has not been inspected yet is validated first. With the boot guard
 * disarmed (the default) this performs the complete staging + IPC preparation
 * and returns; with it armed the ARM9 jumps into the new image and never
 * returns.
 *
 * `path` is the launch path published through `__system_argv`; pass NULL to
 * fall back to `entry->name`.
 */
static void loader_boot_selected(menu_diag_t *d, fat_volume_t *vol,
                                 const fat_file_entry_t *entry,
                                 const char *path)
{
    loader_result_t res;

    d->boot_staged = false;
    d->boot_dry_run = false;

    if (!d->loader_inspected || !d->loader_loadable) {
        loader_inspect(d, vol, entry);
    }
    if (!d->loader_loadable) {
        return;
    }

    (void)loader_stage_and_boot_path(vol, entry, path, d->extended_ram, &res);

    if (res.status != NDS_LOAD_OK) {
        d->loader_status = (u8)res.status;
        d->loader_loadable = false;
        return;
    }

    d->boot_staged = res.staged;
    d->boot_dry_run = res.dry_run;
    d->dldi_present = res.dldi_present;
    d->dldi_patched = res.dldi_patched;
    d->dldi_alloc_kb = res.dldi_alloc_kb;
    dsi_memcpy(d->dldi_io_type, res.dldi_io_type, sizeof(d->dldi_io_type));
}

/*
 * NAND / HiyaCFW fallback: when the SD card yields no boot target, probe the
 * internal NAND, mount its FAT partition and locate the DSi System Menu
 * launcher. Read-only: the launcher is only located and reported, never
 * written, so the anti-brick guard (the project conventions) is preserved.
 */
static bool menu_try_nand_boot(menu_diag_t *d)
{
    nand_boot_context_t ctx;
    nand_boot_target_t tgt;
    u32 base;

    d->nand_launcher = false;
    d->nand_launcher_path[0] = '\0';

    diag_nand(d);
    if (!d->nand_ok) {
        return false;
    }

    ctx.sd_card_present = ((d->sd_flags & IPC_SD_FLAG_PRESENT) != 0u);
    ctx.sd_target_found = false;
    ctx.menu_forced = false;
    ctx.nand_present = d->nand_fat;

    boot_resolve_nand_target(&ctx, &tgt);
    if (!tgt.attempt) {
        d->last_action = tgt.reason;
        return false;
    }

    base = (d->nand_format == NAND_TABLE_TWL) ? d->nand_fat_lba : 0u;
    if (!arm9_nand_mount_fat(&s_nand_vol, base)) {
        d->last_action = "NAND: FAT mount failed";
        return false;
    }
    if (!arm9_nand_find_launcher(&s_nand_vol, d->nand_launcher_path,
                                 sizeof(d->nand_launcher_path))) {
        d->last_action = "NAND: no System Menu";
        return false;
    }

    d->nand_launcher = true;
    d->last_action = "NAND System Menu located";
    return true;
}

/*
 * Menu item [8]: locate the resolved autoboot target (from BOOT.CFG, or the
 * classic BOOT.NDS/HELLO.NDS fallback) on the SD root, validate it and stage
 * the dual-CPU handoff. Discovery runs on demand if the startup probe has not
 * already populated the target; when no SD target exists the NAND System Menu
 * redirect is attempted instead.
 */
static void menu_autoboot(menu_diag_t *d)
{
    d->boot_staged = false;
    d->boot_dry_run = false;
    d->last_action = "Autoboot";

    if (!s_autoboot_entry_valid) {
        /*
         * Re-discovery uses the target captured at startup, not the button held
         * now (the A press that selected this menu item must not be misread as
         * a startup hotkey).
         */
        menu_probe_autoboot(d, &s_boot_target);
    }
    if (!s_autoboot_entry_valid) {
        /* No SD target: fall back to the internal NAND System Menu. */
        (void)menu_try_nand_boot(d);
        return;
    }

    loader_boot_selected(d, &s_fat_vol, &s_autoboot_entry, s_autoboot_path);
    if (d->boot_staged) {
        d->last_action = d->boot_dry_run ? "Autoboot dry-run OK"
                                         : "Autoboot handoff";
    } else {
        d->last_action = "Autoboot rejected";
    }
}

/* Scratch list used to filter the /NDS subdirectory before display. */
static fat_file_entry_t s_dir_list[MENU_BROWSE_MAX];

/* Append the mounted root's .nds/.srl files (path = plain 8.3 name). */
static void menu_browse_collect_root(fat_volume_t *vol, u32 *count)
{
    fat_dir_iter_t it;
    fat_file_entry_t e;

    fat_dir_iter_init(vol, &it, true);
    while (*count < MENU_BROWSE_MAX && fat_dir_next(&it, &e)) {
        dsi_memcpy(&s_fat_files[*count], &e, sizeof(e));
        menu_join_path(s_fat_paths[*count], BOOT_CFG_PATH_MAX, NULL, e.name);
        (*count)++;
    }
}

/*
 * Append the /NDS subdirectory's .nds/.srl files, prefixed as "NDS/<NAME>".
 * Missing directory, file-as-directory and empty directories are all no-ops.
 */
static void menu_browse_collect_subdir(fat_volume_t *vol, u32 *count)
{
    fat_file_entry_t dir_entry;
    u32 n = 0u;
    u32 i;

    if (*count >= MENU_BROWSE_MAX) {
        return;
    }
    if (!fat_find_path(vol, MENU_SUBDIR_NAME, &dir_entry) || !dir_entry.is_dir) {
        return;
    }

    (void)fat_list_dir_cluster(vol, dir_entry.start_cluster, s_dir_list,
                               MENU_BROWSE_MAX, &n);

    for (i = 0u; i < n && *count < MENU_BROWSE_MAX; i++) {
        if (s_dir_list[i].is_dir || !fat_is_nds_entry(&s_dir_list[i])) {
            continue;
        }
        dsi_memcpy(&s_fat_files[*count], &s_dir_list[i], sizeof(s_dir_list[i]));
        menu_join_path(s_fat_paths[*count], BOOT_CFG_PATH_MAX,
                       MENU_SUBDIR_PREFIX, s_dir_list[i].name);
        (*count)++;
    }
}

/* Tracks whether the graphical banner overlay is currently drawn. */
static bool s_banner_overlay;

/* Remove the graphical banner overlay (icon + double-size title), if present. */
static void clear_banner_overlay(void)
{
    if (!s_banner_overlay) {
        return;
    }
    console_select(CONSOLE_MAIN);
    ui_font_clear(0, 0, CONSOLE_COLS - UI_FONT_ICON_COLS, 2);
    ui_font_clear(CONSOLE_COLS - UI_FONT_ICON_COLS, 0,
                  UI_FONT_ICON_COLS, UI_FONT_ICON_ROWS);
    s_banner_overlay = false;
}

/*
 * Draw the top-screen banner panel for the highlighted .nds: the decoded 32x32
 * banner icon plus the double-size title. Shares the console's Engine-A BG0
 * layer, so it is redrawn after render_telemetry() every frame. The title is
 * drawn first and the icon on top so a long title cannot clip the thumbnail.
 */
static void render_banner_overlay(const menu_diag_t *d)
{
    const u8 *raw;

    if (d->loader_inspected && d->banner_present && d->banner_crc_ok &&
        (raw = loader_banner_raw()) != NULL) {
        console_select(CONSOLE_MAIN);
        ui_font_draw_title16(0, 0, CONSOLE_PAL_CYAN, d->banner_title);
        ui_font_draw_icon(CONSOLE_COLS - UI_FONT_ICON_COLS, 0, raw,
                          UI_FONT_BPP_4);
        s_banner_overlay = true;
        return;
    }
    clear_banner_overlay();
}

/*
 * Mount the SD card's FAT volume and browse the .nds / .srl files in its root
 * directory plus inside the optional one-level /NDS subdirectory.
 *
 *   UP/DOWN  move the selection
 *   Y or SELECT  verify / dry-run inspect the highlighted image
 *   A        stage the image and trigger the dual-CPU handoff
 *   B        return to the main menu
 */
static void menu_browse_view(menu_diag_t *d)
{
    input_state_t keys = { 0u, 0u, 0u };
    u32 count = 0u;
    u32 cursor = 0u;
    bool mounted = false;
    const char *fail_reason = NULL;

    d->fat_ok = false;
    d->boot_loaded = false;
    d->loader_inspected = false;
    d->loader_loadable = false;
    d->boot_staged = false;
    d->boot_dry_run = false;
    d->banner_present = false;
    d->banner_crc_ok = false;
    d->banner_title[0] = '\0';
    d->sd_fs_type = FAT_TYPE_NONE;
    d->sd_total_sectors = 0u;

    /* Refresh the SD/TMIO status so the diagnostic is specific. */
    diag_query_sd(d);

    if ((d->sd_flags & IPC_SD_FLAG_UNAVAILABLE) != 0u) {
        fail_reason = "SD UNAVAILABLE (DS MODE)";
    } else if ((d->sd_flags & IPC_SD_FLAG_PRESENT) == 0u) {
        fail_reason = "NO SD CARD MOUNTED";
    } else {
        mounted = arm9_storage_mount(&s_fat_vol, IPC_STORAGE_DEV_SD);
        if (!mounted) {
            fail_reason = "MOUNT FAILED (NOT FAT16/32)";
        }
    }

    if (mounted) {
        menu_browse_collect_root(&s_fat_vol, &count);
        menu_browse_collect_subdir(&s_fat_vol, &count);
        d->fat_ok = true;
        d->sd_fs_type = (u8)s_fat_vol.type;
        d->sd_total_sectors = s_fat_vol.total_sectors;
    }

    /*
     * Seed the edge detector so the A press that opened the browser is not
     * seen as a fresh press on the first frame.
     */
    input_poll(&keys);

    for (;;) {
        u32 i;

        render_telemetry(d);
        render_banner_overlay(d);

        console_select(CONSOLE_SUB);
        console_clear();
        console_puts(0, 0, CONSOLE_PAL_CYAN, "SD CARD / BOOT FILES");

        if (fail_reason != NULL) {
            console_puts(0, 2, CONSOLE_PAL_RED, fail_reason);
        } else if (count == 0u) {
            console_puts(0, 2, CONSOLE_PAL_YELLOW, "NO .NDS/.SRL FILES");
        } else {
            for (i = 0u; i < count; i++) {
                char line[CONSOLE_COLS + 1];

                console_format(line, sizeof(line), "%c%s",
                               (i == cursor) ? '>' : ' ', s_fat_paths[i]);
                console_puts(0, 2 + (int)i,
                             (i == cursor) ? CONSOLE_PAL_YELLOW
                                           : CONSOLE_PAL_WHITE,
                             line);
            }
        }

        console_puts(0, 20, CONSOLE_PAL_GREY,
                     "U/D SEL Y CHECK A BOOT B BACK");

        wait_vblank();

        diag_poll_extkey();
        input_poll(&keys);

        if (input_pressed(&keys, INPUT_B)) {
            break;
        }
        if (count == 0u) {
            continue;
        }
        if (input_pressed(&keys, INPUT_DOWN)) {
            cursor = (cursor + 1u) % count;
        }
        if (input_pressed(&keys, INPUT_UP)) {
            cursor = (cursor + count - 1u) % count;
        }
        if (input_pressed(&keys, INPUT_Y) ||
            input_pressed(&keys, INPUT_SELECT)) {
            loader_inspect(d, &s_fat_vol, &s_fat_files[cursor]);
            d->last_action = "Verify .nds";
        }
        if (input_pressed(&keys, INPUT_A)) {
            loader_boot_selected(d, &s_fat_vol, &s_fat_files[cursor],
                                 s_fat_paths[cursor]);
            d->last_action = d->boot_staged ? (d->boot_dry_run
                                                   ? "Boot dry-run OK"
                                                   : "Boot handoff")
                                            : "Boot rejected";
        }
    }

    clear_banner_overlay();
    d->last_action = "SD browse";
}

/*
 * Menu item [10]: the NAND installer. This is the only code path in the
 * firmware permitted to write the eMMC, and it is compiled into the image only
 * under ENABLE_NAND_WRITE_DANGER (the project conventions). In the locked build
 * installer_run() reports INSTALLER_ERR_DISABLED and nothing is written.
 *
 * The modal progress screen is driven by the installer's own progress hook
 * (installer_ui_update), so the six stages PREFLIGHT -> BACKUP ->
 * VERIFY_BACKUP -> UNLOCK -> WRITE -> VERIFY_WRITE and the backup/write
 * percentages update live as the engines stream. The outcome is either the
 * success screen or a high-visibility abort screen that names the exact
 * failure stage and points at the mandatory NANDBACK.BIN recovery file.
 */
static void menu_install_view(menu_diag_t *d)
{
#ifdef ENABLE_RETAIL_NAND_WRITE
    /* DANGER: the retail test build gates every run behind an explicit
     * L+R+START hold so a retail write can never happen by accident. */
    if (!installer_ui_confirm_retail()) {
        d->last_action = "installer cancelled";
        return;
    }
#endif
    d->install_ran = true;
    d->last_action = "NAND installer";

    installer_ui_begin();
    installer_set_progress_hook(installer_ui_update, NULL);

    installer_run(&d->install);

    /* Detach before painting the outcome so no further updates arrive. */
    installer_set_progress_hook(NULL, NULL);

    if (d->install.state == INSTALLER_STATE_DONE) {
        installer_ui_show_success(&d->install);
    } else {
        installer_ui_show_abort(&d->install);
    }

    d->last_action = installer_state_text(d->install.state);

    /* B returns to the main menu (which redraws the telemetry + menu). */
    installer_ui_wait_back();
}

static void menu_execute(const menu_state_t *state, menu_diag_t *d)
{
    switch (menu_action_for(state)) {
    case MENU_ACTION_SELFTEST:
        diag_collect(d);
        d->last_action = "IPC/HW self-test";
        break;
    case MENU_ACTION_INSPECT:
        menu_inspect_view(d);
        d->last_action = "Register inspect";
        break;
    case MENU_ACTION_SD:
        diag_query_sd(d);
        diag_sd_read(d);
        d->last_action = "SD/TMIO LBA0 read";
        break;
    case MENU_ACTION_LZ77:
        diag_lz77(d);
        d->last_action = "LZ77 verify";
        break;
    case MENU_ACTION_HALT:
        menu_halt();
        break;
    case MENU_ACTION_BROWSE:
        menu_browse_view(d);
        break;
    case MENU_ACTION_SCFG_INIT: {
        bool configured = scfg_init();

        diag_collect(d);
        d->last_action = configured ? "SCFG/MBK initialized"
                                    : "SCFG init: N/A (DS mode)";
        break;
    }
    case MENU_ACTION_AUTOBOOT:
        menu_autoboot(d);
        break;
    case MENU_ACTION_INSPECT_CFG:
        menu_bootcfg_view(d);
        break;
    case MENU_ACTION_INSTALL:
        menu_install_view(d);
        break;
    default:
        break;
    }
}

/* ---- Main loop -------------------------------------------------------- */

void menu_run(void)
{
    menu_state_t state;
    menu_diag_t diag;
    input_state_t keys;

    menu_state_init(&state, MENU_ITEM_COUNT);
    input_init();
    ui_font_init();
    keys.held = 0u;
    keys.down = 0u;
    keys.up = 0u;

    diag_collect(&diag);
    diag.last_action = "Boot self-test";

    /*
     * Unlaunch-style hotkey policy: read BOOT.CFG (if present) and the buttons
     * held at startup. B or SELECT forces the interactive menu; otherwise the
     * mapped target (or DEFAULT) is probed. With ENABLE_AUTOBOOT the probe
     * result is acted on immediately.
     */
    menu_load_boot_cfg(&diag);

    /* Apply the BOOT.CFG theme (or the AMOLED defaults) and stage any splash. */
    menu_load_theme(&diag);

    /*
     * Capture the startup hotkeys: the ARM7-forwarded EXTKEYIN bits (X/Y) are
     * queried once, then input_poll() merges them with KEYINPUT so the
     * BOOT.CFG X/Y mappings are visible to the resolver.
     */
    diag_poll_extkey();
    input_poll(&keys);
    boot_resolve_target(keys.held, &s_boot_cfg, &s_boot_target);
    menu_probe_autoboot(&diag, &s_boot_target);
#ifdef ENABLE_AUTOBOOT
    if (s_boot_target.policy == BOOT_POLICY_AUTOBOOT && s_autoboot_entry_valid) {
        menu_autoboot(&diag);
    }
#endif

    for (;;) {
        diag_poll_extkey();
        input_poll(&keys);

        if (input_pressed(&keys, INPUT_DOWN)) {
            menu_state_move(&state, 1);
        }
        if (input_pressed(&keys, INPUT_UP)) {
            menu_state_move(&state, -1);
        }
        if (input_pressed(&keys, INPUT_B)) {
            diag_collect(&diag);
            diag.last_action = "Self-test (B)";
        }
        if (input_pressed(&keys, INPUT_A)) {
            menu_execute(&state, &diag);
        }

        render_telemetry(&diag);
        render_menu(&state);

        wait_vblank();
    }
}
