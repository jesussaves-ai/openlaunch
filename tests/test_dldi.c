/*
 * tests/test_dldi.c
 *
 * Host-side unit tests for the clean-room DLDI scanner and in-RAM patcher
 * (src/common/dldi.c). Exercises:
 *
 *   - dldi_find_offset(): magic present/absent, 4-byte alignment, and the
 *     "no room for a header" rejection;
 *   - dldi_parse_header(): field decode, the 10..16 allocated_size_log2 bounds,
 *     the "must fit the remaining image" check and string sanitisation;
 *   - dldi_apply_patch(): the header address-table / function-pointer
 *     relocation, the FIX_ALL / FIX_GLUE / FIX_GOT in-range pointer walks, the
 *     FIX_BSS zero-fill, and the driver_size_log2 > allocated_size_log2
 *     rejection;
 *   - the built-in read-only driver template (IOType "DSiR").
 *
 * Build/run with:  make host-test
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/dldi.h"

/* Arbitrary driver link base; it cancels out of every relocation. */
#define TEST_BASE 0x00010000u

static void w32(u8 *p, u32 off, u32 v)
{
    p[off + 0u] = (u8)(v & 0xFFu);
    p[off + 1u] = (u8)((v >> 8) & 0xFFu);
    p[off + 2u] = (u8)((v >> 16) & 0xFFu);
    p[off + 3u] = (u8)((v >> 24) & 0xFFu);
}

static u32 r32(const u8 *p, u32 off)
{
    return (u32)p[off + 0u] | ((u32)p[off + 1u] << 8) |
           ((u32)p[off + 2u] << 16) | ((u32)p[off + 3u] << 24);
}

/* Write the 12-byte DLDI signature at `off`. */
static void put_sig(u8 *p, u32 off)
{
    w32(p, off + DLDI_OFF_MAGIC, DLDI_MAGIC);
    p[off + DLDI_OFF_STRING + 0u] = ' ';
    p[off + DLDI_OFF_STRING + 1u] = 'C';
    p[off + DLDI_OFF_STRING + 2u] = 'h';
    p[off + DLDI_OFF_STRING + 3u] = 'i';
    p[off + DLDI_OFF_STRING + 4u] = 's';
    p[off + DLDI_OFF_STRING + 5u] = 'h';
    p[off + DLDI_OFF_STRING + 6u] = 'm';
    p[off + DLDI_OFF_STRING + 7u] = 0x00u;
}

/* A default "no interface" target stub, as shipped in the test payload. */
static void build_target(u8 *buf, u32 len, u32 off)
{
    memset(buf, 0, len);
    put_sig(buf, off);
    buf[off + DLDI_OFF_VERSION] = 1u;
    buf[off + DLDI_OFF_DRIVER_SIZE] = 10u;
    buf[off + DLDI_OFF_FIX_FLAGS] = 0u;
    buf[off + DLDI_OFF_ALLOC_SIZE] = 10u;
    memcpy(buf + off + DLDI_OFF_NAME, "Default (No interface)", 22u);
    memcpy(buf + off + DLDI_OFF_IO_TYPE, "NONE", 4u);
}

/*
 * A synthetic replacement driver. `data/glue/got/bss` sections are laid out on
 * TEST_BASE with one in-range and one out-of-range pointer in the data range so
 * the relocation walks can be asserted precisely.
 */
static void build_driver(u8 *buf, u32 len, u8 fix_flags,
                         u8 drv_log2, u8 alloc_log2)
{
    memset(buf, 0, len);
    put_sig(buf, 0u);
    buf[DLDI_OFF_VERSION] = 1u;
    buf[DLDI_OFF_DRIVER_SIZE] = drv_log2;
    buf[DLDI_OFF_FIX_FLAGS] = fix_flags;
    buf[DLDI_OFF_ALLOC_SIZE] = alloc_log2;
    memcpy(buf + DLDI_OFF_NAME, "Test Driver", 11u);

    w32(buf, DLDI_OFF_DATA_START, TEST_BASE);
    w32(buf, DLDI_OFF_DATA_END, TEST_BASE + 0x100u);
    w32(buf, DLDI_OFF_GLUE_START, TEST_BASE + 0x100u);
    w32(buf, DLDI_OFF_GLUE_END, TEST_BASE + 0x200u);
    w32(buf, DLDI_OFF_GOT_START, TEST_BASE + 0x200u);
    w32(buf, DLDI_OFF_GOT_END, TEST_BASE + 0x300u);
    w32(buf, DLDI_OFF_BSS_START, TEST_BASE + 0x300u);
    w32(buf, DLDI_OFF_BSS_END, TEST_BASE + 0x400u);

    memcpy(buf + DLDI_OFF_IO_TYPE, "TEST", 4u);
    w32(buf, DLDI_OFF_FEATURES, 0x00000001u);

    w32(buf, DLDI_OFF_STARTUP, TEST_BASE + 0x80u);
    w32(buf, DLDI_OFF_IS_INSERTED, TEST_BASE + 0x88u);
    w32(buf, DLDI_OFF_READ, TEST_BASE + 0x90u);
    w32(buf, DLDI_OFF_WRITE, TEST_BASE + 0x98u);
    w32(buf, DLDI_OFF_CLEAR, TEST_BASE + 0xA0u);
    w32(buf, DLDI_OFF_SHUTDOWN, TEST_BASE + 0xA8u);

    /* In-range (needs relocating) and out-of-range (must be left alone). */
    w32(buf, 0x80u, TEST_BASE + 0x08u);
    w32(buf, 0x84u, 0xDEADBEEFu);

    /* Glue and GOT have one in-range pointer each. */
    w32(buf, 0x110u, TEST_BASE + 0x120u);
    w32(buf, 0x210u, TEST_BASE + 0x220u);

    /* BSS must be zero-filled by FIX_BSS. */
    memset(buf + 0x300u, 0xAA, 0x100u);
}

/* ------------------------------------------------------------------ */

static void test_find_offset(void)
{
    u8 buf[512];
    u32 off = 0xFFFFFFFFu;

    memset(buf, 0, sizeof(buf));
    assert(!dldi_find_offset(buf, sizeof(buf), &off));
    assert(off == 0xFFFFFFFFu);   /* untouched on failure */

    put_sig(buf, 0x04u);
    assert(dldi_find_offset(buf, sizeof(buf), &off));
    assert(off == 0x04u);

    /* Scan finds the first match. */
    put_sig(buf, 0x40u);
    assert(dldi_find_offset(buf, sizeof(buf), &off) && off == 0x04u);

    /* A signature that does not sit on a 4-byte boundary is ignored. */
    {
        u8 b2[512];

        memset(b2, 0, sizeof(b2));
        put_sig(b2, 0x02u);
        assert(!dldi_find_offset(b2, sizeof(b2), &off));
    }

    /* No room for a whole header after the signature -> not a DLDI section. */
    {
        u8 b3[128];

        memset(b3, 0, sizeof(b3));
        put_sig(b3, 0x04u);
        assert(!dldi_find_offset(b3, sizeof(b3), &off));
    }

    /* Degenerate arguments. */
    assert(!dldi_find_offset(NULL, 512u, &off));
}

static void test_parse_header(void)
{
    u8 buf[4096];
    dldi_info_t info;

    build_target(buf, sizeof(buf), 0u);
    assert(dldi_parse_header(buf, sizeof(buf), &info));
    assert(info.version == 1u);
    assert(info.driver_size_log2 == 10u);
    assert(info.allocated_size_log2 == 10u);
    assert(info.driver_size == 1024u);
    assert(info.allocated_size == 1024u);
    assert(strcmp(info.io_type, "NONE") == 0);
    assert(strcmp(info.friendly_name, "Default (No interface)") == 0);

    /* No magic. */
    build_target(buf, sizeof(buf), 0u);
    buf[0] ^= 0xFFu;
    assert(!dldi_parse_header(buf, sizeof(buf), &info));

    /* allocated_size_log2 out of the sane bounds. */
    build_target(buf, sizeof(buf), 0u);
    buf[DLDI_OFF_ALLOC_SIZE] = 20u;
    assert(!dldi_parse_header(buf, sizeof(buf), &info));
    build_target(buf, sizeof(buf), 0u);
    buf[DLDI_OFF_ALLOC_SIZE] = 9u;
    assert(!dldi_parse_header(buf, sizeof(buf), &info));

    /* The reserved block must fit the remaining image. */
    build_target(buf, sizeof(buf), 0u);
    assert(!dldi_parse_header(buf, 512u, &info));   /* 1024 > 512 */

    /* Too short to even hold a header. */
    assert(!dldi_parse_header(buf, DLDI_HEADER_SIZE - 1u, &info));

    /* Degenerate arguments leave the output zeroed. */
    memset(&info, 0xAA, sizeof(info));
    assert(!dldi_parse_header(NULL, 1024u, &info));
    assert(info.version == 0u);
}

static void test_scan(void)
{
    u8 buf[4096];
    dldi_info_t info;
    u32 off = 0u;

    build_target(buf, sizeof(buf), 0x100u);
    assert(dldi_scan(buf, sizeof(buf), &info, &off));
    assert(off == 0x100u);
    assert(strcmp(info.io_type, "NONE") == 0);

    memset(buf, 0, sizeof(buf));
    assert(!dldi_scan(buf, sizeof(buf), &info, &off));
}

static void test_apply_all_fixes(void)
{
    u8 target[4096];
    u8 driver[1024];
    const u32 stub_off = 0x200u;
    const u32 ram_base = 0x02000000u;
    u32 delta = ram_base + stub_off - TEST_BASE;
    u32 i;

    build_target(target, sizeof(target), stub_off);
    build_driver(driver, sizeof(driver),
                 DLDI_FIX_ALL | DLDI_FIX_GLUE | DLDI_FIX_GOT | DLDI_FIX_BSS,
                 10u, 10u);

    assert(dldi_apply_patch(target, sizeof(target), ram_base,
                            driver, sizeof(driver)));

    /* The header address table is rebased to the destination. */
    assert(r32(target, stub_off + DLDI_OFF_DATA_START) == TEST_BASE + delta);
    assert(r32(target, stub_off + DLDI_OFF_DATA_END) ==
           TEST_BASE + 0x100u + delta);
    assert(r32(target, stub_off + DLDI_OFF_BSS_END) ==
           TEST_BASE + 0x400u + delta);

    /* The six IO function pointers are rebased too. */
    assert(r32(target, stub_off + DLDI_OFF_STARTUP) ==
           TEST_BASE + 0x80u + delta);
    assert(r32(target, stub_off + DLDI_OFF_IS_INSERTED) ==
           TEST_BASE + 0x88u + delta);
    assert(r32(target, stub_off + DLDI_OFF_READ) == TEST_BASE + 0x90u + delta);
    assert(r32(target, stub_off + DLDI_OFF_WRITE) == TEST_BASE + 0x98u + delta);
    assert(r32(target, stub_off + DLDI_OFF_CLEAR) == TEST_BASE + 0xA0u + delta);
    assert(r32(target, stub_off + DLDI_OFF_SHUTDOWN) ==
           TEST_BASE + 0xA8u + delta);

    /* FIX_ALL relocated the in-range data pointer but not the alien one. */
    assert(r32(target, stub_off + 0x80u) == TEST_BASE + 0x08u + delta);
    assert(r32(target, stub_off + 0x84u) == 0xDEADBEEFu);

    /* FIX_GLUE and FIX_GOT relocated their pointers. */
    assert(r32(target, stub_off + 0x110u) == TEST_BASE + 0x120u + delta);
    assert(r32(target, stub_off + 0x210u) == TEST_BASE + 0x220u + delta);

    /* FIX_BSS zero-filled the BSS range. */
    for (i = 0u; i < 0x100u; i++) {
        assert(target[stub_off + 0x300u + i] == 0x00u);
    }

    /* The driver identity replaced the default stub. */
    assert(memcmp(target + stub_off + DLDI_OFF_IO_TYPE, "TEST", 4u) == 0);
    assert(target[stub_off + DLDI_OFF_ALLOC_SIZE] == 10u);
}

static void test_apply_selective_fixes(void)
{
    u8 target[4096];
    u8 driver[1024];
    const u32 stub_off = 0x200u;
    const u32 ram_base = 0x02000000u;
    u32 i;

    /* Only FIX_ALL: glue/GOT pointers and BSS are left untouched. */
    build_target(target, sizeof(target), stub_off);
    build_driver(driver, sizeof(driver), DLDI_FIX_ALL, 10u, 10u);
    assert(dldi_apply_patch(target, sizeof(target), ram_base,
                            driver, sizeof(driver)));
    assert(r32(target, stub_off + 0x80u) != TEST_BASE + 0x08u);
    assert(r32(target, stub_off + 0x110u) == TEST_BASE + 0x120u);
    assert(r32(target, stub_off + 0x210u) == TEST_BASE + 0x220u);
    for (i = 0u; i < 0x100u; i++) {
        assert(target[stub_off + 0x300u + i] == 0xAAu);
    }

    /* Only FIX_BSS: no pointer walks, but the BSS is cleared. */
    build_target(target, sizeof(target), stub_off);
    build_driver(driver, sizeof(driver), DLDI_FIX_BSS, 10u, 10u);
    assert(dldi_apply_patch(target, sizeof(target), ram_base,
                            driver, sizeof(driver)));
    assert(r32(target, stub_off + 0x80u) == TEST_BASE + 0x08u);
    assert(r32(target, stub_off + 0x110u) == TEST_BASE + 0x120u);
    for (i = 0u; i < 0x100u; i++) {
        assert(target[stub_off + 0x300u + i] == 0x00u);
    }
}

static void test_apply_rejections(void)
{
    u8 target[4096];
    u8 driver[2048];
    const u32 stub_off = 0x200u;
    const u32 ram_base = 0x02000000u;

    /* driver_size_log2 (11) > target allocated_size_log2 (10). */
    build_target(target, sizeof(target), stub_off);
    build_driver(driver, sizeof(driver), 0u, 11u, 11u);
    assert(!dldi_apply_patch(target, sizeof(target), ram_base,
                             driver, sizeof(driver)));

    /* No DLDI section in the target. */
    memset(target, 0, sizeof(target));
    build_driver(driver, sizeof(driver), 0u, 10u, 10u);
    assert(!dldi_apply_patch(target, sizeof(target), ram_base,
                             driver, sizeof(driver)));

    /* Driver shorter than a header. */
    build_target(target, sizeof(target), stub_off);
    assert(!dldi_apply_patch(target, sizeof(target), ram_base,
                             driver, DLDI_HEADER_SIZE - 1u));

    /* Degenerate arguments. */
    assert(!dldi_apply_patch(NULL, sizeof(target), ram_base,
                             driver, sizeof(driver)));
    assert(!dldi_apply_patch(target, sizeof(target), ram_base, NULL,
                             sizeof(driver)));
}

static void test_builtin_driver(void)
{
    const u8 *ro = dldi_builtin_ro_driver();
    dldi_info_t info;
    u8 target[4096];
    const u32 stub_off = 0x100u;
    const u32 ram_base = 0x02004000u;
    u32 delta = ram_base + stub_off - DLDI_BUILTIN_RO_BASE;
    u32 i;

    assert(ro != NULL);
    assert(dldi_parse_header(ro, DLDI_BUILTIN_RO_SIZE, &info));
    assert(info.version == DLDI_VERSION_VALUE);
    assert(info.allocated_size == 1024u);
    assert(info.driver_size == 1024u);
    assert((info.fix_flags & DLDI_FIX_BSS) != 0u);
    assert(strcmp(info.io_type, DLDI_BUILTIN_RO_IO_TYPE) == 0);
    assert(strcmp(info.friendly_name, "DSiR Read-Only Stub") == 0);

    build_target(target, sizeof(target), stub_off);
    assert(dldi_apply_patch(target, sizeof(target), ram_base,
                            ro, DLDI_BUILTIN_RO_SIZE));

    /* The patched stub now advertises the built-in DSiR read-only driver. */
    assert(memcmp(target + stub_off + DLDI_OFF_IO_TYPE, "DSiR", 4u) == 0);
    assert(r32(target, stub_off + DLDI_OFF_STARTUP) ==
           DLDI_BUILTIN_RO_BASE + 0x100u + delta);
    assert(r32(target, stub_off + DLDI_OFF_WRITE) ==
           DLDI_BUILTIN_RO_BASE + 0x130u + delta);

    /* Its BSS is cleared by the FIX_BSS flag. */
    for (i = 0u; i < 0x40u; i++) {
        assert(target[stub_off + 0x3C0u + i] == 0x00u);
    }
}

int main(void)
{
    test_find_offset();
    test_parse_header();
    test_scan();
    test_apply_all_fixes();
    test_apply_selective_fixes();
    test_apply_rejections();
    test_builtin_driver();

    puts("test_dldi: all assertions passed");
    return 0;
}
