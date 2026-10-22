/*
 * src/arm9/installer_ui.c
 *
 * Modal progress + outcome screens for the guarded NAND installer.
 * See include/arm9/installer_ui.h.
 *
 * The pure helpers (stage label/index, progress-bar formatter, failure-stage
 * selection and the recovery line) are host-testable; the drawing entry points
 * use the shared text console and are only ever reached on the ARM9.
 *
 * Nothing here writes the eMMC: the UI only observes the installer engine's
 * progress hook and reports the result (AGENTS.md §5).
 */
#include "arm9/installer_ui.h"
#include "arm9/console.h"
#include "arm9/input.h"
#include "arm9/video.h"

/* The six linear stages, in display order. */
static const installer_state_t s_ui_stages[INSTALLER_UI_STAGE_COUNT] = {
    INSTALLER_STATE_PREFLIGHT,
    INSTALLER_STATE_BACKUP,
    INSTALLER_STATE_VERIFY_BACKUP,
    INSTALLER_STATE_UNLOCK,
    INSTALLER_STATE_WRITE,
    INSTALLER_STATE_VERIFY_WRITE,
};

/* The recovery instruction rendered on the abort screen. */
#define INSTALLER_UI_RECOVERY_TEXT "RESTORE: SD:/NANDBACK.BIN"

/* Zeroed stand-in so the outcome screens never dereference a NULL report. */
static const installer_report_t s_ui_no_report;

/* Redraw throttle state (single-threaded; the installer is blocking). */
static installer_state_t s_ui_state = INSTALLER_STATE_IDLE;
static u8 s_ui_percent;
static bool s_ui_have;

/* ---- Pure helpers (host-testable) ------------------------------------- */

const char *installer_ui_stage_label(installer_state_t state)
{
    switch (state) {
    case INSTALLER_STATE_IDLE:          return "IDLE";
    case INSTALLER_STATE_PREFLIGHT:     return "PREFLIGHT";
    case INSTALLER_STATE_BACKUP:        return "BACKUP NAND";
    case INSTALLER_STATE_VERIFY_BACKUP: return "VERIFY BACKUP";
    case INSTALLER_STATE_UNLOCK:        return "UNLOCK GUARD";
    case INSTALLER_STATE_WRITE:         return "WRITE NAND";
    case INSTALLER_STATE_VERIFY_WRITE:  return "VERIFY WRITE";
    case INSTALLER_STATE_DONE:          return "DONE";
    case INSTALLER_STATE_ABORTED:       return "ABORTED";
    default:                            return "UNKNOWN";
    }
}

int installer_ui_stage_index(installer_state_t state)
{
    switch (state) {
    case INSTALLER_STATE_PREFLIGHT:     return 0;
    case INSTALLER_STATE_BACKUP:        return 1;
    case INSTALLER_STATE_VERIFY_BACKUP: return 2;
    case INSTALLER_STATE_UNLOCK:        return 3;
    case INSTALLER_STATE_WRITE:         return 4;
    case INSTALLER_STATE_VERIFY_WRITE:  return 5;
    case INSTALLER_STATE_DONE:          return (int)INSTALLER_UI_STAGE_COUNT;
    default:                            return -1;
    }
}

int installer_ui_progress_bar(char *dst, size_t cap, u8 percent, u32 width)
{
    u32 i;
    u32 filled;
    u32 n = 0u;

    if (dst == NULL || cap == 0u) {
        return 0;
    }
    if (percent > 100u) {
        percent = 100u;
    }

    /* Reserve room for "[] 100%" (7 chars) + the terminator. */
    {
        u32 max_width = (cap > 8u) ? (u32)(cap - 8u) : 0u;

        if (width > max_width) {
            width = max_width;
        }
        if (width > 24u) {
            width = 24u;
        }
    }

    /* Round to the nearest cell, but never let a non-zero progress read as
     * empty and never let an incomplete pass read as full. */
    filled = (width * (u32)percent + 50u) / 100u;
    if (percent != 0u && filled == 0u) {
        filled = 1u;
    }
    if (percent != 100u && width != 0u && filled >= width) {
        filled = width - 1u;
    }
    if (filled > width) {
        filled = width;
    }

    dst[n++] = '[';
    for (i = 0u; i < width; i++) {
        dst[n++] = (i < filled) ? '#' : '-';
    }
    dst[n++] = ']';
    dst[n] = '\0';

    n += (u32)console_format(dst + n, cap - n, " %3u%%", (unsigned)percent);
    return (int)n;
}

const char *installer_ui_failure_stage(const installer_report_t *rep)
{
    if (rep == NULL || installer_ui_stage_index(rep->failed_state) < 0) {
        /* The installer is defined to fail its pre-flight cascade first, so
         * that is the honest fallback when no stage was recorded. */
        return installer_ui_stage_label(INSTALLER_STATE_PREFLIGHT);
    }
    return installer_ui_stage_label(rep->failed_state);
}

const char *installer_ui_recovery_text(void)
{
    return INSTALLER_UI_RECOVERY_TEXT;
}

/* ---- Hardware rendering (ARM9 only) ----------------------------------- */

static void ui_wait_vblank(void)
{
    while ((REG16(DISPSTAT) & 0x0001u) == 0u) {
        /* wait for VBlank */
    }
    while ((REG16(DISPSTAT) & 0x0001u) != 0u) {
        /* wait for the end of VBlank */
    }
}

/*
 * Draw the six-stage checklist. Stages before `current` are complete ('x'),
 * `current` is in progress ('>') or the failed stage ('!') and the remainder
 * are pending (' ').
 */
static void ui_draw_stage_rows(int current, bool failed)
{
    u32 i;

    console_select(CONSOLE_MAIN);
    for (i = 0u; i < INSTALLER_UI_STAGE_COUNT; i++) {
        char line[CONSOLE_COLS + 1u];
        char mark = ' ';
        u8 pal = CONSOLE_PAL_GREY;

        if ((int)i < current) {
            mark = 'x';
            pal = CONSOLE_PAL_GREEN;
        } else if ((int)i == current) {
            mark = failed ? '!' : '>';
            pal = failed ? CONSOLE_PAL_RED : CONSOLE_PAL_YELLOW;
        }
        console_format(line, sizeof(line), "%c %u/%u %s", mark,
                       (unsigned)(i + 1u),
                       (unsigned)INSTALLER_UI_STAGE_COUNT,
                       installer_ui_stage_label(s_ui_stages[i]));
        console_puts(0, 4 + (int)i, pal, line);
    }
}

void installer_ui_begin(void)
{
    s_ui_state = INSTALLER_STATE_IDLE;
    s_ui_percent = 0u;
    s_ui_have = false;

    console_select(CONSOLE_MAIN);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_CYAN, "OPENLAUNCH NAND INSTALLER");
    console_puts(0, 1, CONSOLE_PAL_RED, "MODAL - DO NOT POWER OFF");
#ifdef ENABLE_NAND_WRITE_DANGER
    console_puts(0, 2, CONSOLE_PAL_RED, "GUARD: WRITE PATH ARMED");
#else
    console_puts(0, 2, CONSOLE_PAL_GREEN, "GUARD: NAND WRITE LOCKED");
#endif
    console_puts(0, 3, CONSOLE_PAL_WHITE, "STAGES:");
    ui_draw_stage_rows(-1, false);

    console_puts(0, 16, CONSOLE_PAL_GREY, "Backup -> verify -> flash");
    console_puts(0, 17, CONSOLE_PAL_GREY, "-> read-back verify.");

    console_select(CONSOLE_SUB);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_CYAN, "INSTALL PROGRESS");
    console_puts(0, 2, CONSOLE_PAL_RED, "DO NOT POWER OFF");
    console_puts(0, 4, CONSOLE_PAL_RED, "DO NOT REMOVE SD CARD");
    console_puts(0, 6, CONSOLE_PAL_GREY, "Keep the DSi plugged in.");
    console_puts(0, 8, CONSOLE_PAL_GREY, "Buttons are ignored until");
    console_puts(0, 9, CONSOLE_PAL_GREY, "the installer finishes.");

    /* Let the skeleton reach the screen before installer_run() blocks. */
    ui_wait_vblank();
}

void installer_ui_update(void *ctx, const installer_progress_t *progress)
{
    char bar[CONSOLE_COLS + 1u];
    int current;

    (void)ctx;
    if (progress == NULL) {
        return;
    }

    /* A counted stream only redraws when the percentage actually moves; a
     * state transition (total == 0) always redraws. */
    if (s_ui_have && progress->state == s_ui_state &&
        progress->total != 0u && progress->percent == s_ui_percent) {
        return;
    }
    s_ui_state = progress->state;
    s_ui_percent = progress->percent;
    s_ui_have = true;

    /* The dedicated abort screen supersedes the progress view. */
    if (progress->state == INSTALLER_STATE_ABORTED) {
        return;
    }

    if (installer_ui_stage_index(progress->state) < 0) {
        return;
    }
    current = installer_ui_stage_index(progress->state);

    ui_draw_stage_rows(current, false);

    console_select(CONSOLE_MAIN);
    (void)installer_ui_progress_bar(bar, sizeof(bar), progress->percent, 20u);
    console_puts(0, 11, CONSOLE_PAL_WHITE, bar);
    console_printf(0, 12, CONSOLE_PAL_CYAN, "STAGE: %s",
                   installer_ui_stage_label(progress->state));
    console_printf(0, 13, CONSOLE_PAL_GREY, "MSG  : %s",
                   (progress->message != NULL) ? progress->message : "-");
    if (progress->total != 0u) {
        console_printf(0, 14, CONSOLE_PAL_GREY, "SEC  : %u/%u",
                       (unsigned)progress->done, (unsigned)progress->total);
    } else {
        console_puts(0, 14, CONSOLE_PAL_GREY, "SEC  : -");
    }
}

void installer_ui_show_abort(const installer_report_t *rep)
{
    const char *stage;
    int idx;

    if (rep == NULL) {
        rep = &s_ui_no_report;
    }
    stage = installer_ui_failure_stage(rep);
    idx = installer_ui_stage_index(rep->failed_state);
    if (idx < 0) {
        /* No usable stage recorded: blame the pre-flight cascade, which is the
         * first stage and the only honest fallback. */
        idx = 0;
    }

    console_select(CONSOLE_MAIN);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_RED, "*** INSTALL FAILED ***");
    console_puts(0, 1, CONSOLE_PAL_RED, "NAND NOT MODIFIED");
    ui_draw_stage_rows(idx, true);

    console_printf(0, 11, CONSOLE_PAL_RED, "ERROR: %s",
                   installer_error_text(rep->error));
    console_printf(0, 12, CONSOLE_PAL_GREY, "DETAIL: %s",
                   (rep->detail[0] != '\0') ? rep->detail : stage);
    console_printf(0, 13, CONSOLE_PAL_GREY, "TOKEN : %s",
                   rep->token_found ? "verified" : "missing/rejected");
    console_printf(0, 14, CONSOLE_PAL_GREY, "BACKUP: %s",
                   rep->backup_ok ? "written+verified" : "not completed");
    console_printf(0, 15, CONSOLE_PAL_GREY, "FLASH : %s",
                   rep->flashed ? "written+verified" : "not written");

    if (!rep->write_enabled) {
        console_puts(0, 17, CONSOLE_PAL_YELLOW, "BUILD: READ-ONLY (AGENTS 5)");
    }
    console_puts(0, 18, CONSOLE_PAL_YELLOW, installer_ui_recovery_text());
    console_puts(0, 19, CONSOLE_PAL_GREY, "Full raw NAND image on the SD.");
    console_puts(0, 20, CONSOLE_PAL_GREY, "Restore it with a PC + NTR boot.");
    console_puts(0, 22, CONSOLE_PAL_CYAN, "B: RETURN TO MENU");

    console_select(CONSOLE_SUB);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_RED, "INSTALL ABORTED");
    console_printf(0, 2, CONSOLE_PAL_YELLOW, "STAGE: %s", stage);
    console_puts(0, 4, CONSOLE_PAL_GREY, "Nothing was written to NAND.");
    console_puts(0, 6, CONSOLE_PAL_WHITE, "RECOVERY FILE:");
    console_puts(0, 7, CONSOLE_PAL_CYAN, "  SD:/NANDBACK.BIN");
    console_puts(0, 9, CONSOLE_PAL_GREY, "Full raw NAND image written");
    console_puts(0, 10, CONSOLE_PAL_GREY, "before any flash. Keep it.");
    console_puts(0, 14, CONSOLE_PAL_CYAN, "B: RETURN TO MENU");
}

void installer_ui_show_success(const installer_report_t *rep)
{
    if (rep == NULL) {
        rep = &s_ui_no_report;
    }

    console_select(CONSOLE_MAIN);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_GREEN, "*** INSTALL COMPLETE ***");
    console_puts(0, 2, CONSOLE_PAL_GREEN, "PAYLOAD: WRITTEN + VERIFIED");
    console_puts(0, 4, CONSOLE_PAL_GREY, "All six stages passed:");
    console_puts(0, 5, CONSOLE_PAL_GREEN, "  SHA-256 backup + read-back OK.");
    console_printf(0, 7, CONSOLE_PAL_GREY, "DETAIL: %s",
                   (rep->detail[0] != '\0') ? rep->detail : "installed OK");
    console_puts(0, 9, CONSOLE_PAL_YELLOW, "SAFETY COPY KEPT:");
    console_puts(0, 10, CONSOLE_PAL_CYAN, "  SD:/NANDBACK.BIN");
    console_puts(0, 12, CONSOLE_PAL_GREY, "Reboot to run the new image.");
    console_puts(0, 14, CONSOLE_PAL_CYAN, "B: RETURN TO MENU");

    console_select(CONSOLE_SUB);
    console_clear();
    console_puts(0, 0, CONSOLE_PAL_GREEN, "INSTALL COMPLETE");
    console_puts(0, 2, CONSOLE_PAL_WHITE, "NAND write verified.");
    console_puts(0, 4, CONSOLE_PAL_GREY, "Backup: SD NANDBACK.BIN");
    console_puts(0, 8, CONSOLE_PAL_CYAN, "B: RETURN TO MENU");
}

void installer_ui_wait_back(void)
{
    input_state_t keys = { 0u, 0u, 0u };

    /* Seed the edge detector so the A press that opened [10] is not seen as a
     * fresh press on the first frame. */
    input_poll(&keys);
    for (;;) {
        ui_wait_vblank();
        input_poll(&keys);
        if (input_pressed(&keys, INPUT_B)) {
            return;
        }
    }
}
