/*
 * tools/dsi_nand_tool.c
 *
 * Host-side offline tool for the DSi NAND: decrypt/encrypt a raw eMMC image
 * with the per-console key (src/common/dsi_crypto.c) and report the layout.
 *
 * This is the offline half of the Unlaunch-style installer: it proves the crypto
 * over the *whole* image (not just spot sectors) and produces a decrypted image
 * that can be mounted for inspection. It never touches a console.
 *
 * Usage:
 *   dsi_nand_tool info     <image>
 *   dsi_nand_tool decrypt  <in> <out>
 *   dsi_nand_tool encrypt  <in> <out>
 *   dsi_nand_tool verify   <image>          (decrypt+re-encrypt == identity)
 *
 * The CID and CPU/Console ID are read from the no$gba/Unlaunch footer at byte
 * 0xFF800 ("DSi eMMC CID/CPU"); if absent, pass --cid HEX --console HEX.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/dsi_crypto.h"

#define FOOTER_OFF 0xFF800u
#define FOOTER_MAGIC "DSi eMMC CID/CPU"
#define CHUNK (1u << 20)   /* 1 MiB: multiple of 16 */

static int hexbyte(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex(const char *s, u8 *out, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        int hi = hexbyte(s[2 * i]), lo = hexbyte(s[2 * i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (u8)((hi << 4) | lo);
    }
    return s[2 * n] == '\0' ? 0 : -1;
}

static int load_keys(const char *path, u8 cid[16], u8 console_id[8],
                     int have_keys)
{
    FILE *fh = fopen(path, "rb");
    u8 buf[0x40];

    if (!fh) { perror("open"); return -1; }
    if (fseek(fh, (long)FOOTER_OFF, SEEK_SET) != 0 ||
        fread(buf, 1, sizeof(buf), fh) != sizeof(buf)) {
        fclose(fh);
        return have_keys ? 0 : -1;
    }
    fclose(fh);
    if (memcmp(buf, FOOTER_MAGIC, 16) == 0) {
        memcpy(cid, buf + 0x10, 16);
        memcpy(console_id, buf + 0x20, 8);
        return 1;
    }
    return have_keys ? 0 : -1;
}

static int transform(const char *in, const char *out,
                     const u8 key[16], const u8 cid[16])
{
    FILE *fi = fopen(in, "rb");
    FILE *fo = fopen(out, "wb");
    u8 *buf = malloc(CHUNK);
    u32 addr = 0;

    if (!fi || !fo || !buf) { perror("open"); return -1; }
    for (;;) {
        size_t n = fread(buf, 1, CHUNK, fi);
        if (n == 0) break;
        if (n & 15u) { fprintf(stderr, "image not 16-byte aligned\n"); return -1; }
        dsi_nand_crypt(key, cid, addr, buf, (u32)n);
        if (fwrite(buf, 1, n, fo) != n) { perror("write"); return -1; }
        addr += (u32)n;
    }
    fclose(fi); fclose(fo); free(buf);
    return 0;
}

static int verify(const char *in, const u8 key[16], const u8 cid[16])
{
    FILE *fi = fopen(in, "rb");
    u8 *a = malloc(CHUNK), *b = malloc(CHUNK);
    u32 addr = 0;
    int ok = 1;

    if (!fi || !a || !b) { perror("open"); return -1; }
    for (;;) {
        size_t n = fread(a, 1, CHUNK, fi);
        if (n == 0) break;
        memcpy(b, a, n);
        dsi_nand_crypt(key, cid, addr, b, (u32)n);   /* decrypt */
        dsi_nand_crypt(key, cid, addr, b, (u32)n);   /* re-encrypt */
        if (memcmp(a, b, n) != 0) {
            fprintf(stderr, "round-trip mismatch at 0x%X\n", addr);
            ok = 0; break;
        }
        addr += (u32)n;
    }
    fclose(fi); free(a); free(b);
    return ok ? 0 : -1;
}

static void info(const char *path, const u8 key[16], const u8 cid[16],
                 const u8 console_id[8])
{
    FILE *fh = fopen(path, "rb");
    u8 sec[512];
    u32 lba, size;
    int i;

    printf("key          : ");
    for (i = 0; i < 16; i++) printf("%02x", key[i]);
    printf("\nconsole id   : ");
    for (i = 0; i < 8; i++) printf("%02X ", console_id[i]);
    printf("\ncid          : ");
    for (i = 0; i < 16; i++) printf("%02X ", cid[i]);
    printf("\n");

    if (!fh) { perror("open"); return; }
    if (fread(sec, 1, 512, fh) != 512) { fclose(fh); return; }
    fclose(fh);
    dsi_nand_crypt(key, cid, 0u, sec, 512u);
    printf("MBR sig      : %02X%02X\n", sec[0x1FE], sec[0x1FF]);
    for (i = 0; i < 4; i++) {
        const u8 *e = sec + 0x1BE + i * 16;
        memcpy(&lba, e + 8, 4);
        memcpy(&size, e + 12, 4);
        if (e[4] == 0 && lba == 0 && size == 0) continue;
        printf("  part %d      : type=0x%02X lba=0x%X size=0x%X (%u sectors)\n",
               i, e[4], lba, size, size);
    }
}

int main(int argc, char **argv)
{
    u8 cid[16] = {0}, console_id[8] = {0}, key[16];
    int have_keys = 0, i;

    if (argc < 3) {
        fprintf(stderr,
            "usage: %s {info|decrypt|encrypt|verify} <in> [out]\n"
            "       options: --cid HEX --console HEX\n", argv[0]);
        return 2;
    }
    for (i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--cid") == 0 && i + 1 < argc) {
            if (parse_hex(argv[++i], cid, 16) != 0) { fprintf(stderr, "bad --cid\n"); return 2; }
            have_keys++;
        } else if (strcmp(argv[i], "--console") == 0 && i + 1 < argc) {
            if (parse_hex(argv[++i], console_id, 8) != 0) { fprintf(stderr, "bad --console\n"); return 2; }
            have_keys++;
        }
    }
    if (load_keys(argv[2], cid, console_id, have_keys) < 0) {
        fprintf(stderr, "no footer and no --cid/--console supplied\n");
        return 2;
    }
    dsi_nand_key(console_id, key);

    if (strcmp(argv[1], "info") == 0) {
        info(argv[2], key, cid, console_id);
        return 0;
    }
    if (strcmp(argv[1], "verify") == 0) {
        int rc = verify(argv[2], key, cid);
        printf(rc == 0 ? "verify: whole-image round-trip OK\n"
                       : "verify: FAILED\n");
        return rc == 0 ? 0 : 1;
    }
    if (argc < 4) { fprintf(stderr, "missing output path\n"); return 2; }
    if (strcmp(argv[1], "decrypt") == 0) {
        return transform(argv[2], argv[3], key, cid) == 0 ? 0 : 1;
    }
    if (strcmp(argv[1], "encrypt") == 0) {
        return transform(argv[2], argv[3], key, cid) == 0 ? 0 : 1;
    }
    fprintf(stderr, "unknown command: %s\n", argv[1]);
    return 2;
}
