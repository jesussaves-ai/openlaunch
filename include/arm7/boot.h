/*
 * include/arm7/boot.h
 *
 * ARM7 boot entry points referenced by src/arm7/crt0.s.
 */
#ifndef DSI_ARM7_BOOT_H
#define DSI_ARM7_BOOT_H

#include "../common/types.h"

/* C entry reached from the reset vector after BSS is cleared and stacks set. */
void arm7_main(void) __attribute__((noreturn));

/* C target of the IRQ vector (see crt0.s). */
void arm7_irq_handler(void);

#endif /* DSI_ARM7_BOOT_H */
