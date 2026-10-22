/*
 * tests/test_installer_ui.c
 *
 * Host-side unit tests for the hardware-independent half of the installer's
 * modal UI (src/arm9/installer_ui.c):
 *
 *   suite 15  stage labels / indices, the bounded progress-bar formatter, the
 *             failure-stage selector and the recovery line.
 *
 * Only the pure helpers are exercised here: the drawing entry points write to
 * VRAM and are never invoked on the host (they are linked only so the object
 * resolves).
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arm9/installer_ui.h"
#include "common/installer.h"
#include "common/types.h"

/* ------------------------------------------------------------------ */
/* 15a  Stage labels + indices                                         */
/* ------------------------------------------------------------------ */

static void test_stage_labels(void)
{
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_PREFLIGHT),
                  "PREFLIGHT") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_BACKUP),
                  "BACKUP NAND") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_VERIFY_BACKUP),
                  "VERIFY BACKUP") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_UNLOCK),
                  "UNLOCK GUARD") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_WRITE),
                  "WRITE NAND") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_VERIFY_WRITE),
                  "VERIFY WRITE") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_DONE),
                  "DONE") == 0);
    assert(strcmp(installer_ui_stage_label(INSTALLER_STATE_ABORTED),
                  "ABORTED") == 0);

    /* Out-of-range values must never read past the table. */
    assert(installer_ui_stage_label((installer_state_t)99) != NULL);
}

static void test_stage_index(void)
{
    assert(installer_ui_stage_index(INSTALLER_STATE_PREFLIGHT) == 0);
    assert(installer_ui_stage_index(INSTALLER_STATE_BACKUP) == 1);
    assert(installer_ui_stage_index(INSTALLER_STATE_VERIFY_BACKUP) == 2);
    assert(installer_ui_stage_index(INSTALLER_STATE_UNLOCK) == 3);
    assert(installer_ui_stage_index(INSTALLER_STATE_WRITE) == 4);
    assert(installer_ui_stage_index(INSTALLER_STATE_VERIFY_WRITE) == 5);
    assert(installer_ui_stage_index(INSTALLER_STATE_DONE) ==
           (int)INSTALLER_UI_STAGE_COUNT);
    assert(installer_ui_stage_index(INSTALLER_STATE_IDLE) == -1);
    assert(installer_ui_stage_index(INSTALLER_STATE_ABORTED) == -1);
    assert(installer_ui_stage_index((installer_state_t)99) == -1);
}

/* ------------------------------------------------------------------ */
/* 15b  Progress-bar formatter                                         */
/* ------------------------------------------------------------------ */

static void test_progress_bar(void)
{
    char buf[64];

    assert(installer_ui_progress_bar(buf, sizeof(buf), 0u, 10u) ==
           (int)strlen("[----------]   0%"));
    assert(strcmp(buf, "[----------]   0%") == 0);

    assert(installer_ui_progress_bar(buf, sizeof(buf), 50u, 10u) > 0);
    assert(strcmp(buf, "[#####-----]  50%") == 0);

    assert(installer_ui_progress_bar(buf, sizeof(buf), 100u, 10u) ==
           (int)strlen("[##########] 100%"));
    assert(strcmp(buf, "[##########] 100%") == 0);

    /* A non-zero percentage never rounds down to an empty bar. */
    assert(installer_ui_progress_bar(buf, sizeof(buf), 1u, 10u) > 0);
    assert(strcmp(buf, "[#---------]   1%") == 0);

    /* An incomplete pass never rounds up to a full bar. */
    assert(installer_ui_progress_bar(buf, sizeof(buf), 99u, 10u) > 0);
    assert(strcmp(buf, "[#########-]  99%") == 0);

    /* Over-range percentages clamp. */
    assert(installer_ui_progress_bar(buf, sizeof(buf), 200u, 10u) > 0);
    assert(strcmp(buf, "[##########] 100%") == 0);

    /* A degenerate zero-width bar still reports the percentage. */
    assert(installer_ui_progress_bar(buf, sizeof(buf), 0u, 0u) > 0);
    assert(strcmp(buf, "[]   0%") == 0);

    /* A tight buffer clamps the bar width rather than overflowing. */
    assert(installer_ui_progress_bar(buf, 8u, 100u, 10u) < 8);
    assert(strcmp(buf, "[] 100%") == 0);

    /* NULL destination / zero capacity are no-ops. */
    assert(installer_ui_progress_bar(NULL, sizeof(buf), 50u, 10u) == 0);
    assert(installer_ui_progress_bar(buf, 0u, 50u, 10u) == 0);
}

/* ------------------------------------------------------------------ */
/* 15c  Failure stage + recovery line                                  */
/* ------------------------------------------------------------------ */

static void test_failure_stage(void)
{
    installer_report_t rep;

    /* No report / no usable stage: the pre-flight cascade is the fallback. */
    assert(strcmp(installer_ui_failure_stage(NULL), "PREFLIGHT") == 0);

    memset(&rep, 0, sizeof(rep));
    rep.failed_state = INSTALLER_STATE_IDLE;
    assert(strcmp(installer_ui_failure_stage(&rep), "PREFLIGHT") == 0);

    rep.failed_state = INSTALLER_STATE_PREFLIGHT;
    assert(strcmp(installer_ui_failure_stage(&rep), "PREFLIGHT") == 0);

    rep.failed_state = INSTALLER_STATE_BACKUP;
    assert(strcmp(installer_ui_failure_stage(&rep), "BACKUP NAND") == 0);

    rep.failed_state = INSTALLER_STATE_VERIFY_BACKUP;
    assert(strcmp(installer_ui_failure_stage(&rep), "VERIFY BACKUP") == 0);

    rep.failed_state = INSTALLER_STATE_WRITE;
    assert(strcmp(installer_ui_failure_stage(&rep), "WRITE NAND") == 0);

    /* The terminal ABORTED state carries no stage: fall back. */
    rep.failed_state = INSTALLER_STATE_ABORTED;
    assert(strcmp(installer_ui_failure_stage(&rep), "PREFLIGHT") == 0);
}

static void test_recovery_text(void)
{
    const char *t = installer_ui_recovery_text();

    assert(t != NULL);
    assert(strstr(t, "NANDBACK.BIN") != NULL);
}

int main(void)
{
    test_stage_labels();
    test_stage_index();
    test_progress_bar();
    test_failure_stage();
    test_recovery_text();

    puts("test_installer_ui: all assertions passed");
    return 0;
}
