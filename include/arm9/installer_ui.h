/*
 * include/arm9/installer_ui.h
 *
 * Modal progress / outcome UI for the guarded NAND installer (menu item [10]).
 *
 * The installer engines in src/arm9/installer_backup.c and
 * src/arm9/installer_install.c are deliberately hardware-free: they report
 * their progress through the callback installed with
 * installer_set_progress_hook() (see include/common/installer.h). This module
 * supplies that callback, walking the user through the six linear stages
 *
 *     PREFLIGHT -> BACKUP -> VERIFY_BACKUP -> UNLOCK -> WRITE -> VERIFY_WRITE
 *
 * and then a dedicated success or high-visibility abort screen. The abort
 * screen names the exact failure stage and instructs the user to recover from
 * the mandatory NANDBACK.BIN full-NAND backup on the SD card.
 *
 * The stage labels, the stage index and the progress-bar formatter are pure and
 * exercised by tests/test_installer_ui.c. The drawing entry points use the
 * shared text console and are only ever called on the ARM9.
 */
#ifndef DSI_ARM9_INSTALLER_UI_H
#define DSI_ARM9_INSTALLER_UI_H

#include "../common/types.h"
#include "../common/installer.h"
#include "installer.h"

/* The six stages walked by the modal progress screen. */
#define INSTALLER_UI_STAGE_COUNT 6u

/* Upper-case display label for one installer state ('UNKNOWN' when invalid). */
const char *installer_ui_stage_label(installer_state_t state);

/*
 * Position of `state` in the linear stage list: 0..5 for
 * PREFLIGHT..VERIFY_WRITE, INSTALLER_UI_STAGE_COUNT once DONE, or -1 for
 * IDLE/ABORTED/an out-of-range value.
 */
int installer_ui_stage_index(installer_state_t state);

/*
 * Format a "[####----]  50%" progress bar of `width` cells into `dst`. Bounded
 * and NUL-terminated; returns the number of characters written (excluding the
 * terminator). `percent` is clamped to 0..100. Pure.
 */
int installer_ui_progress_bar(char *dst, size_t cap, u8 percent, u32 width);

/* The stage to blame for an aborted report (never NULL; falls back to
 * PREFLIGHT when the report carries no usable stage). Pure. */
const char *installer_ui_failure_stage(const installer_report_t *rep);

/* The fixed recovery instruction line shown on the abort screen. Pure. */
const char *installer_ui_recovery_text(void);

/* ---- Hardware rendering (ARM9 only) ----------------------------------- */

/* Draw the modal progress skeleton on both screens. */
void installer_ui_begin(void);

/* installer_progress_fn-compatible callback: live stage + percentage updates.
 * Redraws only when the stage or the percentage actually changes. */
void installer_ui_update(void *ctx, const installer_progress_t *progress);

/* Draw the success outcome screen. */
void installer_ui_show_success(const installer_report_t *rep);

/* Draw the high-visibility abort screen + NANDBACK.BIN recovery guidance. */
void installer_ui_show_abort(const installer_report_t *rep);

/* Block until the user presses B to return to the main menu. */
void installer_ui_wait_back(void);

#ifdef ENABLE_RETAIL_NAND_WRITE
/*
 * Blocking red DANGER gate for the retail raw-write test build: returns true
 * only when the user holds L + R + START together for ~2 s, false when they
 * press B to cancel. Compiled only into the retail test build
 * (`make installer-retail`).
 */
bool installer_ui_confirm_retail(void);
#endif

#endif /* DSI_ARM9_INSTALLER_UI_H */
