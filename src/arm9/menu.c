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
 * "[5] Reboot / Halt System" action; the eMMC/NAND guard (AGENTS.md §5) is
 * reported from the build configuration.
 */
#include "arm9/menu.h"
#include "arm9/console.h"
#include "arm9/ipc.h"
#include "arm9/mpu.h"
#include "arm9/scfg.h"
#include "arm9/storage.h"
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
};

static const char *const s_menu_desc[MENU_ITEM_COUNT] = {
    "IPC + RTC + SD + FW self-test",
    "Dump SCFG_EXT / SCFG_CLK / MBK",
    "TMIO SD read-only status probe",
    "Expand embedded LZ77 vector",
    "Halt the CPU safely",
    "Mount FAT, list .nds / .srl",
    "Unlock + map DSi SCFG/NWRAM",
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
static fat_file_entry_t s_fat_files[MENU_BROWSE_MAX];
static u8             s_header_buf[FAT_SECTOR_SIZE];

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

    console_puts(0, 0, CONSOLE_PAL_CYAN, "OPENUNLAUNCH C99 BOOTLOADER");
    console_puts(0, 1, CONSOLE_PAL_GREY, "CLEAN-ROOM TWL DIAGNOSTICS");

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

    if (d->sd_total_sectors != 0u) {
        console_printf(0, 16, CONSOLE_PAL_GREY, "SDCAP: %u MiB",
                       (unsigned)((d->sd_total_sectors + 2047u) / 2048u));
    }

    if (d->boot_loaded) {
        console_printf(0, 15,
                       d->boot_crc_ok ? CONSOLE_PAL_GREEN : CONSOLE_PAL_RED,
                       "BOOT : %s %s %s", d->boot_title, d->boot_code,
                       d->boot_crc_ok ? "CRC-OK" : "CRC-BAD");
    } else {
        console_puts(0, 15, CONSOLE_PAL_GREY, "BOOT : -");
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

    console_puts(0, 8, CONSOLE_PAL_GREY, "UP/DOWN SELECT  A RUN");
    console_puts(0, 9, CONSOLE_PAL_GREY, "B: RE-RUN SELF-TEST");
    console_puts(0, 11, CONSOLE_PAL_CYAN, "INFO:");
    console_puts(0, 12, CONSOLE_PAL_WHITE, s_menu_desc[state->cursor]);
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
 * Mount the SD card's FAT volume and browse the .nds / .srl files in its root
 * directory. A loads the selected file's NDS header (via the delegated FAT
 * reader), validates its CRC16 and publishes the title/game code to the top
 * screen; B returns to the main menu.
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
        fat_dir_iter_t it;

        fat_dir_iter_init(&s_fat_vol, &it, true);
        while (count < MENU_BROWSE_MAX &&
               fat_dir_next(&it, &s_fat_files[count])) {
            count++;
        }
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
                               (i == cursor) ? '>' : ' ', s_fat_files[i].name);
                console_puts(0, 2 + (int)i,
                             (i == cursor) ? CONSOLE_PAL_YELLOW
                                           : CONSOLE_PAL_WHITE,
                             line);
            }
        }

        console_puts(0, 20, CONSOLE_PAL_GREY, "UP/DOWN SELECT  A LOAD  B BACK");

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
        if (input_pressed(&keys, INPUT_A)) {
            u32 got = 0u;

            if (fat_read_file(&s_fat_vol, &s_fat_files[cursor],
                              s_header_buf, (u32)sizeof(s_header_buf), &got)) {
                nds_header_t h;

                if (nds_header_parse(s_header_buf, got, &h)) {
                    dsi_memcpy(d->boot_title, h.title, sizeof(h.title));
                    dsi_memcpy(d->boot_code, h.gamecode, sizeof(h.gamecode));
                    d->boot_crc_ok = h.crc_ok;
                    d->boot_loaded = true;
                }
            }
        }
    }

    d->last_action = "SD browse";
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
    keys.held = 0u;
    keys.down = 0u;
    keys.up = 0u;

    diag_collect(&diag);
    diag.last_action = "Boot self-test";

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
