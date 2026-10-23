# Hardware & Reverse-Engineering Notes

**OpenLaunch** (`https://github.com/jesussaves-ai/openlaunch`) is a clean-room,
open-source C99 alternative to Unlaunch for the Nintendo DSi. This document
records the raw evidence extracted from `asm/UNLAUNCH.DSI` (the reference
low-level DSi bootloader). Every claim here is reproducible from the file;
nothing is invented.

---

## 1. Resolved `DAT_*` constants (raw image evidence)

The Ghidra export refers to named globals such as `DAT_0001ce84`. These are
literal words stored inside the ARM7 image; reading them from the raw file
yields hardware base pointers. The table below was produced by directly
decoding little-endian 32-bit words at those file offsets.

| Symbol          | Value        | Interpretation                                   |
| :-------------- | :----------- | :----------------------------------------------- |
| `DAT_0001ce84`  | `0x04000000` | I/O base; functions use `+0x1C0` = SPI control    |
| `DAT_0001ce90`  | `0x04000000` | I/O base; functions use `+0x180` = IPC sync       |
| `DAT_0001dca0`  | `0x04004A00` | **SDIO (wifi) controller base** (see §4/§20)      |
| `DAT_0001dca8`  | `0x04004100` | NDMA engine block (shared by SD/MMC and SDIO)     |
| `DAT_0001dcc0`  | `0x04004100` | NDMA engine block (same, `+0x1C` status)          |
| `DAT_0001db4c`  | `0x04004A00` | SDIO controller − init (`FUN_0001d154`)           |
| `DAT_0001db54`  | `0x04004C00` | SDIO auxiliary control (`+0x04`)                  |
| `DAT_0001db38`  | `0x02FD846C` | RAM variable (boot-stage counter)                 |
| `DAT_0001db34`  | `0x02FD8470` | RAM structure                                     |
| `DAT_0001db30`  | `0x02FD8471` | RAM buffer                                        |
| `DAT_0001dc08`  | `0x03808010` | ARM7-WRAM scratch buffer used to build MMC cmds   |
| `DAT_0001dc50`  | `0x03808010` | ARM7-WRAM scratch buffer                          |
| `DAT_00020cec`  | `0x03FFFFFC` | top-of-DTCM word                                  |
| `DAT_00020cf0`  | `0x060245A0` | constant written there (JMP vector / entry)       |
| `DAT_00020d00`  | `0x00C2E66F` | Timer reload/control word (`FUN_00020cd0`)        |
| `DAT_000278d4`  | `0x04808000` | TWL secondary interrupt controller base           |
| `DAT_00022354`  | *(not needed yet)* | per-slot table used by `FUN_0002204c` (`+0x34` stride) |

## 2. Interrupt registers (ARM7)

Confirmed by `FUN_00020ae4`, `FUN_00020af8`, `FUN_00020b04`, `FUN_00020cb0`,
`FUN_0001df84`:

| Address      | Name      | Observed use                                             |
| :----------- | :-------- | :------------------------------------------------------- |
| `0x04000208` | `IME`     | `FUN_00020ae4`: read + clear (critical section enter)    |
|              |           | `FUN_00020af8`: restore                                   |
| `0x04000210` | `IE`      | `FUN_00020b04`: `IE |= mask` (enable)                     |
| `0x04000214` | `IF`      | `FUN_00020cb0`: `IF = 0x40` (acknowledge)                |
| `0x0400021C` | `IE2`?    | `FUN_0001df84`: `= 0x800`                                |

IRQ bit `0x40` (bit 6) is used together with the word written to
`0x0400010C` → this is **Timer 3** (`FUN_00020cd0`).

`FUN_00027028` runs from the TWL secondary interrupt block at `0x04808000`:
it reads `[0x10] & [0x12]`, ORs the collected bits, and writes them back to
`[0x10]` — the standard "pending & enable, then acknowledge" pattern.

## 3. IPC bridge (ARM9 ⇄ ARM7)

| Address      | Name          | Observed use                                          |
| :----------- | :------------ | :---------------------------------------------------- |
| `0x04000180` | `IPCSYNC`     | `FUN_0001cb38`: set `|= 0x4000`, poll low nibble      |
| `0x04000184` | `IPCFIFOCNT`  | `FUN_0001cc34`: poll bit 1 (send-**full**)            |
| `0x04000188` | `IPCFIFOSEND` | `FUN_0001cc34`, `FUN_00020be4`                        |
| `0x04100000` | `IPCFIFORECV` | ARM9→ARM7 receive FIFO                                |

`IPCFIFOCNT` (GBATEK "IPC Fifo Control Register") is **core-relative**: each CPU
sees its own send/receive FIFO status. The full bit map is:

| Bit | Dir | Meaning                                                   |
| :-- | :-- | :-------------------------------------------------------- |
| 0   | R   | Send FIFO empty (`1` = empty)                             |
| 1   | R   | Send FIFO full (`1` = full)                               |
| 2   | R/W | Send FIFO empty IRQ enable                                |
| 3   | W   | Send FIFO clear                                           |
| 8   | R   | Receive FIFO empty (`1` = empty)                          |
| 9   | R   | Receive FIFO full (`1` = full)                            |
| 10  | R/W | Receive FIFO not-empty IRQ enable                         |
| 14  | R/W | Error (read-on-empty / send-on-full)                      |
| 15  | R/W | FIFO master enable                                        |

`FUN_0001cc34`/`FUN_0602c008` poll bit **1** (`and #2`), i.e. they wait until the
send FIFO is *not full* before writing `IPCFIFOSEND`. The send-**empty** status is
bit **0**. (An earlier revision of `dsi_hw.h` defined `SEND_EMPTY` as bit 1 and
`SEND_FULL` as bit 2, so `arm9_ipc_wait_send_empty()` actually spun on
"send-full" and deadlocked on the very first IPC request; see §18.)

`FUN_00020be4(channel, word)` writes `word | (channel << 28)`. The high nibble
selects the logical channel; this matches the opcodes seen in the command
dispatcher `FUN_0001f21c` (see `include/common/ipc_protocol.h`).

The dispatcher recognises command words starting with
`0x1001/0x1002/0x1003/0x1004/0x1006/0x1007/0x1008/0x100A/0x100D/0x1010/0x1025`
and `0x3007/0x3008`, plus raw command `0x0E`.

## 4. TMIO SD/MMC + SDIO register block (`0x04004800` / `0x04004A00`)

The DSi has two TMIO-compatible controllers with an **identical register
layout**:

| Base         | Controller | Slots                                          |
| :----------- | :--------- | :--------------------------------------------- |
| `0x04004800` | SD/MMC     | external SD card (port 0) + onboard eMMC (port 1) |
| `0x04004A00` | SDIO       | Atheros wifi                                   |

> **Important correction.** Every TMIO access in the reference ARM7 image uses
> `0x04004A00` (verified by disassembling the whole ARM7 slice — `0x04004800`
> never appears). The `FUN_0001d930`/`d958`/`d750`/`d6e4`/`d760` family is
> therefore the **SDIO/wifi** driver (it issues SDIO `CMD52`/`CMD53`), not the
> SD/eMMC reader. The register model is shared, so the same engine is applied
> to the SD/MMC controller in `src/arm7/sdmmc.c` with standard SD commands.

Register offsets (observed in the reference and cross-checked against the
dsibrew "SD/MMC/SDIO Registers" page and melonDS `DSi_SD.cpp`):

| Offset | Width | Name                 | Observed use                                 |
| :----- | :---- | :------------------- | :------------------------------------------- |
| `0x00` | 16    | `SD_CMD`             | command index/type/response/data flags       |
| `0x02` | 16    | `SD_CARD_PORT_SELECT`| bit0 selects SD card vs eMMC                 |
| `0x04` | 32    | `SD_CMD_PARAM`       | command argument (`*(uint*)(base+2)`)        |
| `0x08` | 16    | `SD_STOP_INTERNAL`   | stop / auto-CMD12 (`puVar1[4]`)              |
| `0x0A` | 16    | `SD_DATA16_BLK_COUNT`| transfer block count (`puVar1[5]`)           |
| `0x0C` | 16×8  | `SD_RESPONSE0-7`     | 128-bit response                             |
| `0x1C` | 32    | `SD_IRQ_STATUS`      | interrupt status / acknowledge               |
| `0x20` | 32    | `SD_IRQ_MASK`        | interrupt mask (1=disable)                   |
| `0x24` | 16    | `SD_CARD_CLK_CTL`    | SDCLK enable + divider                       |
| `0x26` | 16    | `SD_DATA16_BLK_LEN`  | block length (`puVar1[0x13]`)                |
| `0x28` | 16    | `SD_CARD_OPTION`     | bus width + timeouts                         |
| `0x30` | 16    | `SD_DATA16_FIFO`     | 16-bit data port (`puVar1[0x18]`)            |
| `0xD8` | 16    | `SD_DATA_CTL`        | 16/32-bit data mode (`puVar1[0x6c]`)         |
| `0xE0` | 16    | `SD_SOFT_RESET`      | soft reset                                   |
| `0x100`| 16    | `SD_DATA32_IRQ`      | DATA32 IRQ control (`puVar1[0x80]`)          |
| `0x104`| 16    | `SD_DATA32_BLK_LEN`  | DATA32 block length (`puVar1[0x82]`)         |
| `0x108`| 16    | `SD_DATA32_BLK_COUNT`| DATA32 block count (`puVar1[0x84]`)          |
| `0x10C`| 32    | `SD_DATA32_FIFO`     | 32-bit data port (`puVar1[0x86]`)            |

`SD_IRQ_STATUS` bits used by the polling loops (`*(uint*)(base+0x1C)`):

* `0x00000001` — `CMDRESPEND` (response end / command done).
* `0x00000004` — `DATAEND` (last data block end).
* `0x007F0000` — error mask: CMD-index, CRC, end-bit, data-timeout,
  RX-overflow, TX-underrun, command-timeout (non-zero ⇒ abort).
* `0x01000000` — `RXRDY` (read FIFO not empty).
* `0x02000000` — `TXRQ` (write FIFO wants data).
* `0x80000000` — `ILA` (illegal command access).
* Writing `status & 0x7C80FFFA` acknowledges the flags.

NDMA block `0x04004100` (used by the reference DMA path): `[0x04]` = source,
`[0x08]` = destination, `[0x0C]` = word count, `[0x10]` = 0x20, `[0x1C]` =
trigger/status (`& 0x80000000` = busy).

> **Safety** — the project conventions requires eMMC to be read-only by default.
> `src/arm7/sdmmc.c` therefore only implements the read/`CMD` path; the
> write helper is compiled out unless `ENABLE_NAND_WRITE_DANGER` is defined.

## 5. SPI bus (`0x040001C0`)

`FUN_0001c96c(dst_ctrl, ctrl, keep_irq)`:

1. `IME = 0` (disable IRQs while the bus is bit-banged by the CPU).
2. `SPICNT = ctrl | (chip_select << 16)`.
3. Poll `SPICNT & 0x80` (busy) until clear.
4. If `keep_irq == 1`, run `SWI 3` and restore `IME = 1`.
5. Return the byte at `SPIDATA` (`+0x02`).

`FUN_0001ca10` shifts a firmware address out three bytes at a time
(`uVar1 = 0x18; uVar1 -= 8` loop) — a 24-bit firmware read command.

## 6. RTC (`0x04000138`)

`FUN_0001cf1c`, `FUN_0001cf40`, `FUN_0001cf54`, `FUN_0001cf60` bit-bang the
RTC/PMIC transport at `0x04000138`:

* `0x73` written to select/idle the bus.
* Bit transmission: value `0x74 | bit` then `+2` (clock toggle), with a short
  `SWI 3` delay between edges.
* `FUN_0001cf1c` additionally pokes `0x77` — register select step.

This transport is shared with the BPTWL power-management IC. the project conventions
restricts writes to documented registers only.

## 7. Memory primitives

`FUN_0001c72c` (memset) and `FUN_0001c7d0` (memcpy) are hand-written,
word-aligned implementations (10-word unrolled inner loop, 1/2/4-byte
prologue/epilogue). `FUN_0001ca50` = `strlen`, `FUN_0001caa8` = `memcmp`.

`FUN_0001cae0` performs the ARM9/ARM7 cache maintenance via CP15:
`Clean Data Cache by index` (loop over sets), `Data Synchronization Barrier`,
`Invalidate Entire Instruction cache`, `Invalidate Entire Data cache`.

## 8. Stack / entry (ARM7)

The ARM7 image begins at file `0xF800` with hand-written assembly:

```
mov  r0, #0x04000000     ; I/O base
msr  cpsr_c, #0xD3       ; enter SVC, IRQ/FIQ off
sub  sp, r0, #0x40       ; sp = 0x03FFFFC0
...
```

The genuine ARM7 exception vector table is therefore at the load address
`0x02380000` (and a copy in ARM7 WRAM), **not** at file offset `0x04`.

Correction (subsequent pass): the ARM9 image is **not** encrypted. It is a small
crt0 stub followed by a graphics blob and one Nintendo LZ77 (type `0x10`) code
segment. It has been fully unpacked (see §9) and decompiled into
`asm/decompiled/arm9/` (271 functions) by `scripts/run_ghidra.sh`.

---

## 9. ARM9 image layout (resolved)

`asm/UNLAUNCH.DSI`'s NDS header points the ARM9 image at file `0x4000`
(load `0x02200000`, entry `0x02200800`, size `0xAD80`). The raw slice is *not*
flat executable code:

| File offset (ARM9-relative) | Size      | Contents                                  |
| :-------------------------- | :-------- | :---------------------------------------- |
| `0x0000` – `0x0800`         | `0x800`   | opaque header / padding (never executed)  |
| `0x0800` – `0x08F0`         | `0xF0`    | ARM9 crt0 stub (**`@ref crt0 0x02200800`**) |
| `0x08F0` – `0x4580`         | `0x3C90`  | raw graphics/asset blob                   |
| `0x4580` – `0xAD80`         | `0x6800`  | Nintendo LZ77 (type `0x10`) code segment  |

The crt0 (`scripts/unpack_unlaunch_arm9.py` reproduces it exactly) does:

1. CP15: `DSB`, invalidate I-cache, invalidate D-cache, then
   `control = 0x00057078`.
2. Wait for `IPCSYNC` (`0x04000180`) low nibble `== 0xE`, then write `0x4E00`.
3. Write `SCFG_EXT` (`0x04004008`) `= 0x8307F100`.
4. VRAM bank map: `[0x04000240] = 0x80848981`, `[0x04000244] = 0x03000000`,
   `[0x04000248] = 0`.
5. `[0x06030004] = 0xFF`.
6. BIOS **SWI #0x11** (LZ77) expand `0x02204580` → `0x02100000`
   (uncompressed size `0x931C`).
7. `memcpy(0x02100000 -> 0x06023C90, 0x931C)` (the code).
8. `memcpy(0x022008F0 -> 0x06020000, 0x3C90)` (the graphics).
9. `ldr pc,[0x022008EC]` → jump to **`0x0602572C`**.

The reconstructed run-time image is therefore contiguous in VRAM:

| Address      | Size      | Contents                                   |
| :----------- | :-------- | :----------------------------------------- |
| `0x06020000` | `0x3C90`  | graphics / asset segment (data)            |
| `0x06023C90` | `0x931C`  | ARM9 code                                  |
| `0x0602572C` | —         | stage-2 entry (**`@ref entry`**)           |

`asm/arm9_unpack.json` records these values; `scripts/run_ghidra.sh` wraps the
code segment in an ELF (`build/arm9_code.elf`, load `0x06023C90`, entry
`0x0602572C`) so Ghidra's ELF loader and ARMv5T auto-analysis recover the 271
functions.

## 10. ARM9 CP15 / MPU / TCM (`@ref FUN_0602c374`, `0x0602C374`)

The stage-2 boot code programs the ARM946E-S protection unit from a literal
pool at `0x0602C424`. Decoded descriptors:

| CP15 | Region / field | Value        | Base       | Size     |
| :--- | :------------- | :----------- | :--------- | :------- |
| c6,0 | data region 0  | `0x00000035` | `0x00000000` | 128 MiB |
| c6,1 | data region 1  | `0x0200002B` | `0x02000000` | 4 MiB   |
| c6,2 | data region 2  | `0x0600002F` | `0x06000000` | 16 MiB  |
| c6,3 | data region 3  | `0x08000035` | `0x08000000` | 128 MiB |
| c6,4 | data region 4  | `0x0300001B` | `0x03000000` | 16 KiB  |
| c6,5 | data region 5  | `0x0100002F` | `0x01000000` | 16 MiB  |
| c6,6 | data region 6  | `0xFFFF001D` | `0xFFFF0000` | 32 KiB  |
| c6,7 | data region 7  | `0x02C0002B` | `0x02C00000` | 4 MiB   |

Region size = `2^(((desc >> 1) & 0x1F) + 1)`; bit 0 is the enable bit.
Also: `c2 = 0x00000046`, `c3 = 0x00000006`, data access `c5,c0,2 = 0x15111111`,
instruction access `c5,c0,3 = 0x05100111`, `DTCM c9,c1,0 = 0x0300000A`,
`ITCM c9,c1,1 = 0x00000020`. Final `CP15 control |= 0x41005`
(MPU + D-cache + I-cache + ITCM).

## 11. ARM9 hardware registers (resolved)

| Address      | Name            | Evidence / use                                  |
| :----------- | :-------------- | :---------------------------------------------- |
| `0x04000000` | `DISPCNT`       | `entry` forced-blank `0x10000`; engine reset    |
| `0x04000004` | `DISPSTAT`      | cleared by the 2D reset (`memset 0x4A`)         |
| `0x0400000C` | `BG2CNT`        | reset to `0x100` (`@ref entry`)                 |
| `0x04000020` | `BG2HOFS/VOFS`  | `0x1000000` written (`@ref entry` / `FUN_06025df4`) |
| `0x040000B0` | DMA0..3 block   | `entry`: `memset(0x040000B0, 0x40)`             |
| `0x040000D4` | DMA1 SAD/DAD/CNT| SD/MMC DMA path (`FUN_06027c9c`)                |
| `0x04000100` | Timer 0..3      | `entry`: `memset(0x04000100, 0x10)`             |
| `0x04000130` | `KEYINPUT`      | `FUN_06024fa0` polls it while filling VRAM      |
| `0x04000134` | `KEYCNT`        | key IRQ config (`FUN_06024fa0` region)          |
| `0x04000138` | RTC transport   | `FUN_06026698/66bc/66d0/66dc` bit-bang          |
| `0x04000180` | `IPCSYNC`       | crt0 handshake; `FUN_0602bf*`                   |
| `0x04000184` | `IPCFIFOCNT`    | `entry`: `= 0x101`; `FUN_0602c008` polls bit 1 (send-full) |
| `0x04000188` | `IPCFIFOSEND`   | `FUN_0602bff0`: `payload | channel << 28`       |
| `0x040001A0` | `AUXSPICNT`     | firmware/ROM SPI (`FUN_0602b760`)               |
| `0x040001A4` | `AUXSPIDATA`    | `FUN_0602b760`                                  |
| `0x040001A8` | AUX SPI/ROM ctrl| `FUN_0602bd94`                                  |
| `0x040001C0` | `SPICNT`        | `FUN_0602546c/478/484` transfer primitive       |
| `0x040001C2` | `SPIDATA`       | same                                            |
| `0x04000204` | `EXMEMCNT`      | `entry`: `0x6800` then `0xE880`                 |
| `0x04000208` | `IME`           | `entry`: `= 0`; `arm9_irq_unmask_all`           |
| `0x04000210` | `IE`            | `FUN_0602bfb4` (`|=`), `FUN_0602bfc8` (`&= ~`)  |
| `0x04000214` | `IF`            | `entry`: `= 0xFFFFFFFF` (clear all)             |
| `0x04000218` | `IE2`           | `FUN_0602bfdc` (`&= ~`)                         |
| `0x04000240` | `VRAMCNT_A..D`  | crt0 `0x80848981`; `FUN_0602555c` `0x8A848981`  |
| `0x04000244` | `VRAMCNT_E..H`  | crt0 `0x03000000`                               |
| `0x04000248` | `VRAMCNT_H..I`  | crt0 `0`                                        |
| `0x04004004` | `SCFG_CLK`      | `entry`/`FUN_06027158`: `&= 0xFEFE`             |
| `0x04004008` | `SCFG_EXT`      | crt0: `= 0x8307F100`                            |
| `0x04004100` | auxiliary DMA   | `FUN_0602bf88`: clear `0x74` bytes (DSi NDMA)   |
| `0x05000000` | main palette    | `FUN_06026244`                                  |
| `0x05000400` | sub palette     | `FUN_06026244`                                  |
| `0x0400006C` | `MASTER_BRIGHT` | engine A fade reset to `0` (normal brightness)  |
| `0x0400106C` | `MASTER_BRIGHT_SUB` | engine B fade reset to `0`                  |

Notes:

* The ARM9 stage-2 **does not** program the NWRAM ownership registers
  `MBK1`–`MBK9` (`0x04004040`..`0x04004064`); those are the ARM7's
  responsibility (`src/arm9/scfg.c::scfg_set_mbk` exposes them anyway).
* It also writes `0x04000298`, `0x04000300` and `0x04000304` during boot. Their
  exact semantics are not yet confirmed, so they are intentionally left
  unmodelled rather than guessed at.

## 12. ARM9 function map (translated)

| Reference                        | Behaviour                                | C module / symbol              |
| :------------------------------- | :--------------------------------------- | :----------------------------- |
| `@ref crt0 0x02200800`           | cache/CP15 + SCFG + IPC + segment load   | `arm9_stage1_bootstrap`        |
| `@ref FUN_0602c104`              | `memset` (10-word unrolled)              | `dsi_memset` (common)          |
| `@ref FUN_0602c1a8`              | `memcpy` (10-word unrolled)              | `dsi_memcpy` (common)          |
| `@ref FUN_0602c338`              | full cache flush (CP15 by-index)         | `cache_flush_all` (common)     |
| `@ref FUN_0602c374`              | MPU / TCM / CP15 control                 | `arm9_mpu_configure`           |
| `@ref FUN_0602bfb4`              | `IE |= mask`                             | `arm9_irq_enable`              |
| `@ref FUN_0602bfc8`              | `IE &= ~mask`                            | `arm9_irq_disable`             |
| `@ref FUN_0602bfdc`              | `IE2 &= ~mask`                           | `arm9_irq_ie2_disable`         |
| `@ref FUN_0602c008`              | wait send-FIFO not-full                  | `arm9_ipc_wait_send_empty`     |
| `@ref FUN_0602bff0`              | send FIFO word with channel              | `arm9_ipc_send`                |
| `@ref FUN_06025b30`              | `VRAMCNT_A..I = value`                   | `arm9_vram_set_all_banks`      |
| `@ref FUN_0602555c`              | `VRAMCNT_A..D = 0x8A848981`              | `arm9_vram_configure_boot`     |
| `@ref FUN_0602bf88`              | clear auxiliary DMA block                | `arm9_dma_aux_reset`           |
| `@ref entry 0x0602572C`          | full stage-2 bring-up + handoff          | `arm9_main`                    |

The stage-2 `entry` finishes by copying `0x1DC` bytes from `0x023FEE00` to
`0x060259C0` and tail-calling `*0x023FEE00` — i.e. it hands control to a
payload the boot loader places at `0x023FEE00`. That handoff is not yet
translated (documented as `TODO(re)` in `src/arm9/boot.c`).

---

## 13. Register access widths (audit)

Every MMIO register is now reached at its documented width. The melonDS boot
log showed `unknown ARM9 IO write32 04000300` and `... 04004100`; the first was
a 32-bit store to the 8-bit `POSTFLG` register (which also clobbers `HALTCNT` at
`0x04000301` and can halt the CPU), the second a write to the DSi-only NDMA
block from a DS-mode launch.

| Address      | Name       | Width | Where                       |
| :----------- | :--------- | :---- | :-------------------------- |
| `0x04000000` | `DISPCNT`  | 32    | `arm9/video.c`              |
| `0x04000004` | `DISPSTAT` | 16    | `arm9/menu.c` (VBlank wait) |
| `0x04000008` | `BG0CNT`   | 16    | `arm9/console.c`            |
| `0x04000010` | `BG0HOFS`  | 16    | `arm9/console.c`            |
| `0x04000130` | `KEYINPUT` | 16    | `arm9/input.c`              |
| `0x04000136` | `EXTKEYIN` | 16    | `arm7/ipc.c`                |
| `0x04000138` | RTC bus    | 8     | `arm7/rtc.c`                |
| `0x04000180` | `IPCSYNC`  | 16    | `arm9|arm7/ipc.c`           |
| `0x04000184` | `IPCFIFOCNT`| 16   | `arm9|arm7/ipc.c`           |
| `0x04000188` | `IPCFIFOSEND`| 32  | `arm9|arm7/ipc.c`           |
| `0x04000204` | `EXMEMCNT` | 16    | (not written)               |
| `0x04000208` | `IME`      | 16*   | `arm9|arm7/irq.c`           |
| `0x04000210` | `IE`/`IF`  | 32    | `arm9|arm7/irq.c`           |
| `0x04000300` | `POSTFLG`  | **8** | `arm9/video.c`              |
| `0x04000301` | `HALTCNT`  | 8     | `arm9/menu.c` (halt)        |
| `0x04000304` | `POWCNT1`  | 16    | `arm9/video.c`              |
| `0x0400006C` | `MASTER_BRIGHT`     | 16 | `arm9/video.c`         |
| `0x0400106C` | `MASTER_BRIGHT_SUB` | 16 | `arm9/video.c`         |
| `0x04004000` | `SCFG_A9ROM`| 16   | `arm9/scfg.c`               |
| `0x04004008` | `SCFG_EXT` | 32    | `arm9/scfg.c`               |
| `0x04004100` | NDMA block | 32    | `arm9/dma.c` (gated)        |
| `0x04004800` | TMIO/SDMMC | 16/32 | `arm7/sdmmc.c`              |
| `0x04004A00` | SDIO (wifi)| 16/32 | (not accessed)              |

\* GBATEK lists `IME` as 16-bit on ARM9 and 32-bit on ARM7; only bit 0 is
defined, so a 16-bit access is exact on both cores (the ARM7 upper half is
unused). The previous 32-bit store to `0x04000300` was the bug flagged by the
emulator: `POSTFLG` is 8-bit and `HALTCNT` (which halts the CPU when written
`0x80`) lives at the next byte.

## 14. DSi-mode detection, SCFG unlock and NWRAM (MBK) initialisation

The whole `0x04004000` page is DSi-only. The console-mode probe in
`src/arm9/scfg.c` (and its ARM7 mirror in `src/arm7/ipc.c::ipc_twl_mode()`)
applies the canonical GBATEK test to `SCFG_A9ROM` (`0x04004000`, read-only on
ARM9):

```
  if ((SCFG_A9ROM & 03h) == 01h)  DSi mode  else  NDS mode
```

An **explicit `0x03` means the DSi is running an NDS title** (DS compatibility
mode) and is authoritative: the probe returns *false* even though
`SCFG_EXT.bit31` (the SCFG/MBK access gate) is typically still set by the
firmware. Only when `SCFG_A9ROM` reads `0x00` — the page is hidden on a legacy
DS, or bootstage-1 has not latched the mapping yet — does the probe fall back to
`SCFG_EXT.bit31`. (The previous revision used bit31 as an unconditional
fallback, which mis-reported a DSi running in NDS mode as DSi mode and enabled
DSi-only writes there.)

`scfg_hardware_accessible()` distinguishes "the page exists" (`A9ROM != 0` or
bit31) from "DSi mode" (`A9ROM == 1`); the diagnostic UI shows NWRAM/SD status
whenever the hardware is accessible, and *only* falls back to `N/A (DS mode)`
on a genuine legacy DS.

`scfg_init()` is the one-shot DSi bring-up used by `arm9_main()`:

1. `scfg_unlock()` — make sure `SCFG_EXT.bit31` is set so the
   `0x04004000`–`0x04004063` window is readable/writable (a no-op if already
   set by the firmware);
2. `scfg_apply_boot_config()` — `SCFG_EXT = 0x8307F100`, `SCFG_CLK &= 0xFEFE`;
3. `mbk_init()` — program the NWRAM slot-ownership registers `MBK1`..`MBK5`
   with the canonical DSi mapping (`WRAM-A` → ARM7, `WRAM-B`/`WRAM-C` → ARM9),
   honouring the `MBK9` write-protect fields (read-only on ARM9).

Every step is gated on `scfg_is_twl_mode()`, so on a legacy DS `scfg_init()`,
`scfg_apply_boot_config()`, `scfg_enable_extended_ram()`, `scfg_set_mbk()`,
`mbk_init()` and `arm9_dma_aux_reset()` (the NDMA block at `0x04004100`) are all
skipped. This lets the same ROM boot cleanly in both DS and DSi modes.

The **ARM7** has its own SCFG view and a separate SD/MMC power gate (GBATEK
"DSi Control Registers (SCFG)"):

| Register | Bit | Meaning |
| :--- | :--- | :--- |
| `SCFG_CLK7` (`0x04004004`) | 0 | SD/MMC block clock (0 = stop, 1 = run) |
| `SCFG_EXT7` (`0x04004008`) | 18 | Access to the SD/MMC window (`0x04004800`–`0x040049FF`) |
| `SCFG_EXT7` (`0x04004008`) | 19 | Access to the SDIO/Wi-Fi window (`0x04004A00`–`0x04004BFF`) |

`sdmmc_enable_clock()` (`src/arm7/sdmmc.c`) sets `SCFG_EXT7.bit18` and
`SCFG_CLK7.bit0` before the first TMIO access. It self-gates on
`SCFG_A9ROM & 3 == 1` (DSi mode) exactly like the ARM9 probe, so it is a no-op
on a legacy DS and on a DSi running an NDS title. `SCFG_EXT7.bit18` is
documented as "should be the same as `SCFG_CLK7.bit0`"; the function writes both
so the register window and the block clock can never disagree.

The pure decoders (`scfg_mode_from_a9rom`, `scfg_arm9_clock_hz`,
`scfg_ext_ram_bytes`, `mbk_slot_entry`) are host-tested in
`tests/test_scfg.c`. The clock decode reports 67.03 MHz when `SCFG_CLK.bit0`
is clear and 134.06 MHz when it is set (GBATEK "SCFG_CLK").

## 15. Text console and font

`src/arm9/font8x8.c` is a clean-room 8x8 monochrome font covering ASCII
`0x20..0x7F` (96 glyphs, 8 bytes each, MSB = leftmost pixel, glyphs 5 px wide in
bits 6..2). `src/arm9/console.c` expands each glyph to a 4bpp tile (32 bytes)
and installs both engines as BG mode 0 text layers:

| Engine | Bank | VRAMCNT | Tiles        | Map          | Palette      |
| :----- | :--- | :------ | :----------- | :----------- | :----------- |
| A (top)| A    | `0x81`  | `0x06000000` | `0x06001000` | `0x05000000` |
| B (bot)| C    | `0x84`  | `0x06200000` | `0x06201000` | `0x05000400` |

`BG0CNT = 0x0200` selects character base block 0, 4bpp colour and screen base
block 2 (map at base + `0x1000`). Palette bank 0 is white-on-black; banks 1..6
are cyan, green, yellow, red, grey and blue. The tile packer, map-entry packer
and the bounded `console_format`/`console_format_hex32` formatters are pure and
covered by `tests/test_console.c`.

The reference image instead unpacks its graphics blob (`asm/arm9_gfx.bin`) to
VRAM `0x06020000` and its code segment to `0x06023C90` (see §9); that path is
kept in `src/arm9/boot.c::arm9_stage1_bootstrap()` but is not run by this ROM.

## 16. RTC read protocol (S-35180 / S-35199A01)

`src/arm7/rtc.c` bit-bangs `0x04000138`. The read command byte is
`0x86 | (cmd << 4)` (LSB-first), followed by `n` parameter bytes shifted in with
bit base `0x64`:

| cmd | bytes | contents                                  |
| :-- | :---- | :---------------------------------------- |
| 0   | 1     | status register 1                         |
| 2   | 7     | year, month, day, weekday, hour, min, sec |

`rtc_read_datetime()` reads command 2 and decodes the BCD fields, rejecting
out-of-range values so a floating bus cannot masquerade as a valid clock. This
is exposed to the ARM9 over IPC as `IPC_CMD_GET_RTC`.

## 17. ROM padding and cartridge capacity

`scripts/pack_nds.py` pads the output to a power of two of at least 32 KiB
(`0x8000`) with `0xFF`, so melonDS no longer logs
`bad ROM size ... rounded to ...`. The `0x014` Device Capacity byte is set to
the smallest exponent `c` with `128 KiB << c >= file_size` and the `0x080`
total-used-ROM-size field is set to the padded size; the header CRC (`0x15E`) is
recomputed after those fields are written. `scripts/verify_rom.py` asserts the
size is `>= 0x8000`, a power of two, covered by the device capacity, and that
every byte past the ARM7 payload is `0xFF`.

## 18. IPC FIFO deadlock (fixed) — `IPCFIFOCNT` status bits

**Symptom (melonDS boot log).** The ROM boots, but the log stops right after the
ARM9's DTCM remap:

```
remapping DTCM 3000000 3004000 0 fffff000
Game is now booting
remapping DTCM b000000 b004000 3000000 2ffc000
```

and the console never updates past the initial header: the top screen stays at
`ARM7 : TIMEOUT` and the system appears frozen.

**Diagnosis.** Attaching melonDS' GDB stub (`[Instance0.Gdb] Enabled = true`,
ARM9 port 3333) showed the ARM9 parked in `arm9_ipc_wait_send_empty()` inside
`arm9_ipc_send()`, called from `diag_query_rtc()` on the first request after the
PING. Reading `IPCFIFOCNT` (`0x04000184`) gave `0x8501`:

* bit 0 = 1 → send FIFO **empty**, bit 8 = 1 → receive FIFO empty (idle).
* the code's `IPC_FIFOCNT_SEND_EMPTY` was `BIT(1)`, which is actually the
  *send-full* status bit, so `while ((reg & SEND_EMPTY) == 0)` never exited
  (bit 1 is 0 while the FIFO is not full) → deadlock.

**Root cause.** `include/common/dsi_hw.h` shifted the send-status bits by one:
`SEND_EMPTY` was defined as `BIT(1)` and `SEND_FULL` as `BIT(2)`, and an
invented `SEND_ENABLE` occupied `BIT(0)`. Per GBATEK the layout is
`SEND_EMPTY = BIT(0)`, `SEND_FULL = BIT(1)`, `SEND_IRQ = BIT(2)`,
`SEND_CLEAR = BIT(3)`, `RECV_EMPTY = BIT(8)`, `RECV_FULL = BIT(9)`,
`RECV_IRQ = BIT(10)`, `ERROR = BIT(14)`, `ENABLE = BIT(15)`.

**Fix.** Corrected the bit definitions and `IPC_FIFOCNT_INIT_VALUE`
(`ENABLE | RECV_IRQ`) in `include/common/dsi_hw.h`. No other file referenced the
wrong bits.

**Verification.** After the fix, the GDB stub shows the ARM9 in `menu_run()`
(instead of the IPC wait loop), the ARM7 replies to the PING, and the top-screen
BG map (`0x06001000`) renders `ARM7 : OK (1)` in the green palette bank, with the
bottom-screen menu titles intact.

## 19. ARM7-gated hardware delegation (IPC request/reply)

The TMIO SD/eMMC controller (`0x04004800`/`0x04004A00`) and the firmware SPI
flash (`SPICNT` `0x040001C0`) are reachable only from the ARM7. Rather than let
the ARM9 program those registers directly, every such operation is packaged into
a parameterised IPC request and executed by the ARM7 dispatcher
(`ipc_dispatch_pending`, @ref `FUN_0001f21c`).

### Request framing

A request is a header word followed by its argument words. The header reuses the
high nibble for the logical channel (as before) and reserves bits 16-19 for the
argument count:

```
bits  0-15  opcode
bits 16-19  number of argument words that follow
bits 20-27  reserved (0)
bits 28-31  channel nibble (added by ipc_pack)
```

`ipc_request_header(cmd, n)` builds this; a legacy single-word command is simply
`ipc_request_header(cmd, 0) == cmd`, so the existing telemetry commands are
unchanged. The ARM7 assembler accepts the header and the following words across
one or more `ipc_dispatch_pending()` calls before dispatching the handler.

### Shared-buffer window

A delegated transfer may only name a destination in the low 4 MiB of Main RAM
(`0x02000000`–`0x023FFFFF`), validated by `ipc_addr_is_shared_ram()`. This
excludes DTCM (`0x0B000000`, ARM9-only and not reachable by the ARM7), VRAM and
MMIO, so a malformed request cannot make the ARM7 overwrite its own image or a
hardware register. The ARM9 image's `.bss` (at `0x0220xxxx`) is inside this
window; stack/DTCM buffers are not.

### Delegated commands

| Command                 | Request args                              | Reply (payload)        |
| :---------------------- | :---------------------------------------- | :--------------------- |
| `IPC_CMD_READ_SECTOR`   | device, lba, dst, count (512-byte sectors) | flags, sectors done   |
| `IPC_CMD_READ_FIRMWARE` | address (24-bit), dst, length              | flags, bytes done     |

`device` is `IPC_STORAGE_DEV_SD` or `IPC_STORAGE_DEV_EMMC`; transfers are bounded
to `IPC_STORAGE_MAX_SECTORS` (32) and `IPC_FW_MAX_BYTES` (256). The result-flags
word carries `IPC_IO_FLAG_OK` / `_ERROR` / `_BAD_ADDR` / `_BAD_RANGE` /
`_UNAVAILABLE` (the last on a legacy DS, where TMIO is absent).

### Cache synchronisation

The ARM7TDMI has no data cache, so the ARM7 performs no cache maintenance on the
shared buffer. The ARM9 owns the synchronisation: `src/arm9/storage.c` calls
`dc_flush_range()` before the request (dropping stale lines that could later be
written back over the ARM7's data) and `dc_invalidate_range()` after the reply
(so the next read observes the ARM7's writes).

### Safety

The eMMC path stays read-only: `sdmmc_read()` never writes, and
`IPC_CMD_WRITE_SECTOR` is deliberately left unimplemented until a write path is
reviewed and gated behind `ENABLE_NAND_WRITE_DANGER` (the project conventions).
`spi_firmware_read_buf()` only issues the standard SPI `READ` (0x03) command; it
never programs or erases the flash.

### Verification

The boot self-test (`src/arm9/menu.c`) exercises the path end to end: it asks the
ARM7 for the first eight firmware bytes via `arm9_firmware_read()` and shows the
first word on the top screen as `FW RD: xxxxxxxx (ARM7 IPC)`. The pure request
framing and shared-window validator are covered by `tests/test_mem.c`.

## 20. TMIO SD/MMC block reader (`src/arm7/sdmmc.c`)

`sdmmc_read()` was a returning-error scaffold; it is now a working
single/multi 512-byte block reader built on a clean-room translation of the
reference TMIO engine.

### Reference functions translated

| Reference    | Behaviour                                          | C symbol               |
| :----------- | :------------------------------------------------- | :--------------------- |
| `FUN_0001d154` | controller soft reset / clock / option setup     | `sdmmc_init_controller`|
| `FUN_0001d930` | command + FIFO data transfer (read direction)    | `tmio_issue` / `tmio_rx` |
| `FUN_0001d958` | command + FIFO data transfer (write direction)   | `tmio_issue` / `tmio_tx` |
| `FUN_0001d750` | read transfer into the staging buffer            | `tmio_read_block`      |
| `FUN_0001d6e4` | block-read framing (offset/len -> command+FIFO)  | `sdmmc_read`           |
| `FUN_0001d6b8` | "wait until the card is ready" poll              | `tmio_wait_bits`       |

The reference drives the SDIO controller (see §4); the same engine is applied
to the SD/MMC controller with standard SD commands.

### Transfer sequence (single block, CMD17)

1. `SD_DATA_CTL = 0`, `SD_DATA32_IRQ = 0` (16-bit data mode),
   `SD_DATA16_BLK_LEN = 512`, `SD_DATA16_BLK_COUNT = 1`, `SD_STOP = 0`.
2. Write `SD_CMD_PARAM` = `offset` (byte address) or `offset/512` (block
   address, when `sdmmc_t.block_addressed` is set), acknowledge stale IRQs,
   then write `SD_CMD = 17 | DATA | READ`.
3. Poll `CMDRESPEND` (bit 0), aborting on any error bit; read the R1 response.
4. For each 16-bit word: poll `RXRDY` (bit 24) and read `SD_DATA16_FIFO`; then
   poll `DATAEND` (bit 2).

Multi-sector requests loop CMD17 per block (no CMD12 needed).

### Robustness

Unlike the reference's unbounded `do { } while` loops, every hardware poll is
bounded by `sdmmc_t.timeout` (`TMIO_DEFAULT_TIMEOUT`), so a missing or
unresponsive card can never wedge the ARM7; a timeout or any status error bit
returns `false` and the IPC layer replies `IPC_IO_FLAG_ERROR`.

### Card identification

`sdmmc_card_init()` implements the standard SD/MMC sequence (CMD0, CMD8,
ACMD41/CMD1, CMD2, CMD3, CMD9, CMD7, CMD16), sets `block_addressed` from the OCR
CCS bit and records success in `card_ready`. `sdmmc_read()` still assumes the
card is already selected; the ARM7 IPC layer (`src/arm7/ipc.c::ipc_sd_open()`)
runs `sdmmc_card_init()` exactly once per slot before the first read, and only
after the read-only presence gate has confirmed a card is inserted. Every
command is bounded by `sdmmc_t.timeout`, so a missing or unresponsive card
cannot wedge the ARM7.

### DMA

The reference's DMA branch programs the NDMA engine at `0x04004100`. For a
single 512-byte block the FIFO path is used instead: it is simpler, needs no
NDMA access rights (`SCFG_EXT.NDMA_ARM9`), and keeps the transfer inside the
same bounded polling model. The NDMA register semantics are recorded in §4 for
a future multi-block optimisation.

### Verification

`tests/test_sdmmc.c` mocks the register block (via `-DSDMMC_IO_HOOKS`) and
covers the command serializer, the FIFO block reader, byte/block addressing,
and the timeout/error/partial-block rejection paths. The ARM9 self-test reads
LBA 0 (the MBR) through the full delegated pipeline and reports
`SD RD: OK MBR:YES/NO`.

## 21. Read-only FAT16/FAT32 parser (`src/common/fat_ro.c`)

A zero-allocation, bounds-checked FAT parser used to mount the SD card and list
bootable images. It is backend-agnostic: the caller supplies

```c
typedef bool (*fat_read_sector_fn)(void *ctx, u32 lba, u8 out[512]);
```

On the ARM9 the backend is the ARM7-gated IPC sector read
(`src/arm9/storage.c::arm9_storage_mount`), so the parser inherits the whole
delegation/caching pipeline; on the host it is a plain memory callback.

### Mount

1. Read LBA 0. If it carries the `0x55AA` signature and one of the four
   partition entries has a FAT type (`0x04`, `0x06`, `0x0B`, `0x0C`, `0x0E`)
   with a non-zero start LBA, that LBA is the volume base (MBR).
2. Otherwise LBA 0 is assumed to hold the BPB directly (superfloppy).
3. Parse the BPB (bytes/sector must be 512, sectors/cluster a power of two,
   >= 1 FAT, non-zero total and FAT size) and derive:
   - `fat_start_lba = base + reserved_sectors`
   - `root_start_lba = fat_start_lba + num_fats * fat_size`
   - `data_start_lba = root_start_lba + root_sectors`
   - `cluster_count`, `max_cluster`
4. FAT32 is selected when `root_entry_count == 0`; a volume whose type string
   is `"FAT12"` is rejected (FAT12 is out of scope).

### Cluster chain

`fat_get_next_cluster()` reads the 16-bit (FAT16) or 32-bit (FAT32, masked to
28 bits) entry from the FAT table. End-of-chain is `>= 0xFFF8` (FAT16) or
`>= 0x0FFFFFF8` (FAT32); bad markers (`0xFFF7` / `0x0FFFFFF7`) and out-of-range
clusters terminate traversal. A failed sector read returns `0xFFFFFFFF` and
sets `fat_volume.io_error`.

### Directory listing

`fat_dir_iter_init()` starts at the root: the fixed area for FAT16, or the
cluster chain from `root_cluster` for FAT32. `fat_dir_next()` skips long-file
name entries (attribute `0x0F`), volume labels (`0x08`), deleted entries
(`0xE5`), subdirectories (`0x10`) and, when `nds_only` is set, anything that is
not `.NDS`/`.SRL`. Short 8.3 names are expanded to `NAME.EXT`.

### File reading

`fat_read_file()` walks the cluster chain, copying whole sectors through the
volume's single sector buffer, honouring `file_size` and the caller's buffer
bound. It returns true only when the requested amount was read in full.

### Menu integration

`src/arm9/menu.c` item `[6] Browse SD Card (.nds)` mounts the volume, lists up
to 16 `.nds`/`.srl` files on the bottom screen, and on `A` reads the selected
file's header, validates its CRC16 (`src/common/nds_header.c`) and shows the
title/game code on the top screen.

### Verification

`tests/test_fat_ro.c` synthesizes complete FAT16 and FAT32 volumes (MBR,
BPB, FAT tables, root directory, data clusters) and exercises mounting,
geometry, chain traversal, `.nds` filtering, multi-cluster reads, truncated
chains, mount failures and the NDS header CRC.

## 22. Blank white screens (fixed) — `MASTER_BRIGHT` + DISPCNT display mode

**Symptom (melonDS).** After the DTCM remap the ROM boots, but both screens show
a uniform **white** frame; no header, telemetry or backdrop colour appears.

**Diagnosis (two independent causes).**

1. *Master brightness.* `arm9_video_reset()` clears the engine registers from
   `DISPCNT+4` through `DISPCNT+0x4D`, which does **not** include the
   master-brightness mixers at `0x0400006C` / `0x0400106C`. Direct boot and the
   DSi firmware can leave those 16-bit fades at a full white value, washing out
   an otherwise correct BG mode 0 layer.

2. *DISPCNT display-source bits — the real cause of the persistent white frame.*
   DISPCNT **bits 16-17** select the display source: `0` = "screen off" (the
   panel outputs solid white), `1` = normal 2D/3D composition, `2` = VRAM
   display, `3` = main-memory FIFO. libnds always sets bits 16-17 = 1 via
   `MODE_0_2D` (`0x10000`), and melonDS (`src/GPU_Soft.cpp::DrawScanlineA`)
   fills mode `0` with `0x3F3F3F` — its source even notes *"'screen disabled'
   white is 63,63,63"*. The console/BG setup only wrote
   `DISPCNT = 0x00000100` (bits 16-17 = 0) and the bring-up heartbeat wrote
   `DISPCNT = 0`, so both engines ran in "screen off": the correctly-programmed
   tiles, map and palette in `0x06000000`/`0x06001000`/`0x05000000` were simply
   never composited. The earlier revision also mislabelled bit 16 as forced
   blank; the real forced-blank bit is **bit 7** (melonDS computes
   `ForcedBlank = (DispCnt >> 7) & 1`).

**Fix.**

* Added `arm9_video_set_master_brightness()` (`src/arm9/video.c`), which writes
  `0x0000` (`MASTER_BRIGHT_NORMAL`) with a 16-bit store to both engines.
  `arm9_main()` calls it right after `arm9_video_reset()`, before the VRAM banks
  and palette are programmed.
* `include/arm9/video.h` now defines `DISPCNT_DISPLAY_2D` (bits 16-17 = 1) and
  `DISPCNT_FORCED_BLANK` as bit 7. `console_engine_init()` writes
  `DISPCNT_DISPLAY_2D | 0x00000100` (BG0 on), `arm9_video_boot_heartbeat()`
  writes `DISPCNT_DISPLAY_2D`, and `arm9_video_reset()` writes
  `DISPCNT_DISPLAY_2D | DISPCNT_FORCED_BLANK`.

**Verification.** `make all`, `make verify` and `make host-test` stay green; the
BG map (`0x06001000`) now renders against a black backdrop instead of a white
frame, and the heartbeat backdrop colour is visible when `arm9_main()` is
reached.

## 23. SD/MMC command sent to an empty slot (fixed)

**Symptom (melonDS, no SD image configured).** The ARM7 emits
`SD/MMC: SENDING CMD 1811 TO NULL DEVICE` and the UI appears to stall. The log
value `0x1811` is the raw `SD_CMD` word built by `tmio_cmd_word(17, ...)`:
`0x11` (CMD17) with `TMIO_CMD_DATA` (bit 11) and `TMIO_CMD_READ` (bit 12) set.

**Diagnosis.** melonDS constructs port 0 (the external SD card) only when an SD
image is attached (`[DSi.SD] Enable = true`); otherwise `Ports[0]` is null and
`DSi_SDHost::Write(0x000)` logs `... TO NULL DEVICE` and returns *without*
setting `IRQStatus`. A blind `CMD17` therefore never receives `CMDRESPEND` and
the (correctly bounded) `tmio_wait_bits()` poll runs to its full budget before
the ARM7 replies `IPC_IO_FLAG_ERROR`. Issuing a data command into an empty slot
is pointless and delays the reply, so the ARM9 self-test/UI waits needlessly.

**Fix.** `ipc_sd_open()` (`src/arm7/ipc.c`) performs a read-only presence probe
(`sdmmc_get_status()` → `TMIO_ST_SIGSTATE`, bit 5) before any transfer *and*
before the card-identification sequence. If the requested device is the SD card
and no card is inserted, the IPC layer replies `IPC_IO_FLAG_UNAVAILABLE` with
zero sectors transferred and never writes `SD_CMD` (nor issues CMD0/CMD8/etc.).
The onboard eMMC is always populated and skips the gate.

**Verification.** With `[DSi.SD] Enable = false`, `make all` + `make verify` +
`make host-test` stay green and the melonDS run reaches the interactive menu:
the GDB stub shows the ARM9 looping in `render_telemetry` (`console_clear_engine`
/ `render_telemetry`) with the Engine-B BG map (`0x06201000`) populated, and the
ARM7 in its `ipc_dispatch_pending` poll (`0x02380500`) rather than in
`sdmmc_read`. The self-test now reports `SD RD: unavailable` without the
`NULL DEVICE` command.

To make the *enabled* path the default without manual GUI work,
`scripts/configure_emulator.py` writes `[DSi.SD] Enable = true` /
`ImagePath = <repo>/build/sdcard.img` (and DSi console type) into the local
melonDS config; `make run`/`make smoke-test` invoke it automatically. The smoke
test also greps the emulator log for the null-device signature so a
misconfigured card fails loudly.

## 24. TWL header unit code + the TMIO0 read-only probe/browse pipeline

### Cartridge unit code (`0x012`)

`scripts/pack_nds.py` writes the hybrid unit code **`0x02`** (NDS + DSi) for a
`--twl` build (plain `--no-twl` keeps `0x00`):

| Value | Meaning | Launcher behaviour |
| :--- | :--- | :--- |
| `0x00` | NDS only | runs in DS mode only |
| `0x02` | NDS + DSi (hybrid) | a DSi launcher/emulator boots it with native TWL mode active (`SCFG_A9ROM == 0x01`); a legacy DS still runs it |
| `0x03` | DSi-exclusive | DSi only; a legacy DS refuses it |

The hybrid code is what keeps `SCFG_A9ROM` at `0x01` on a DSi (so the SCFG/MBK
and SD/MMC blocks stay accessible) without locking the image to DSi hardware.
The header CRC16 (`0x15E`) is recomputed over `0x000..0x15D` after the field is
written, and `scripts/verify_rom.py` re-checks both CRCs and asserts that a
DSi-capable unit code is exactly the hybrid `0x02` with a non-zero TWL region
mask. `src/common/nds_header.c` also decodes the field (`unit_code`,
`twl_capable`) when the SD browser loads a `.nds` header.

### Read-only probe / browse pipeline

1. **ARM9** `[3] Probe SD / TMIO RO` and `[6] Browse SD Card (.nds)` issue
   `IPC_CMD_GET_SDMMC_STATUS`. The reply is now three payload words:
   `flags` (`IPC_SD_FLAG_*`), `raw` TMIO status and `info`
   (`IPC_SD_INFO_BLOCK_ADDRESSED` = SDHC/SDXC, `IPC_SD_INFO_CARD_READY`).
2. **ARM7** `ipc_reply_sdmmc()` calls `ipc_sd_open()`, which programs the
   controller (`sdmmc_init_controller()` → `sdmmc_enable_clock()` sets
   `SCFG_EXT7.bit18`/`SCFG_CLK7.bit0`), runs the presence gate and then the
   read-only identification sequence (CMD0/CMD8/ACMD41/CMD2/CMD3/CMD9/CMD7/
   CMD16) at most once per slot. The OCR CCS bit selects SDSC (byte) vs
   SDHC/SDXC (block) addressing for `CMD17`.
3. **ARM9** `arm9_storage_probe()` reads LBA 0 through the delegated
   `IPC_CMD_READ_SECTOR`, checks the `0xAA55` boot signature, locates the FAT
   partition with the pure `fat_mbr_find_fat_partition()` helper and mounts the
   volume to classify it FAT16/FAT32 and read its size.
4. The top screen shows presence + `SDSC`/`SDHC`, `MBR:YES/NO`, `FS:FAT16/32`
   and the capacity in MiB; `[6]` reports a specific reason when it cannot
   browse (`SD UNAVAILABLE (DS MODE)`, `NO SD CARD MOUNTED`,
   `MOUNT FAILED (NOT FAT16/32)`).

No write command (`CMD24`/`CMD25`) exists outside the
`ENABLE_NAND_WRITE_DANGER` build, and `IPC_CMD_WRITE_SECTOR` remains
unimplemented, so the whole pipeline is read-only (the project conventions).

### Verification

`tests/test_sdmmc.c` exercises the pure command/status/OCR decoders and the
full identification sequence (SDHC and SDSC), `tests/test_fat_ro.c` covers the
MBR partition scan, FAT16/FAT32 BPB parsing, `.nds`/`.srl` filtering and the
NDS header unit code + CRC16, and `make verify` validates the packaged header.

## 25. Chainloader: header validation, RAM staging and dual-CPU handoff

The SD browser (`[6] Browse SD Card (.nds)`) is now an interactive chainloader.
It never writes NAND/eMMC and only stages an image after the full safety gate
below succeeds.

### 25.1 NDS header validation (`src/common/nds_header.c`)

`nds_header_validate_loadable()` runs over the 512-byte sector-0 header and
returns one of six specific codes (rendered verbatim by the UI):

| Code | Meaning |
| :--- | :--- |
| `NDS_LOAD_OK` | CRC16 matches and every load range is safe |
| `NDS_ERR_BAD_CRC` | stored `0x15E` CRC16 != recomputed Modbus CRC16 |
| `NDS_ERR_ARM9_BOUNDS` | ARM9 offset/entry/load/size zero, unaligned, entry outside the loaded range, or outside Main RAM |
| `NDS_ERR_ARM7_BOUNDS` | the same checks for ARM7 (Main RAM or ARM7 WRAM) |
| `NDS_ERR_OVERLAP_BOOTLOADER` | a range intersects the loader reservation |
| `NDS_ERR_FILE_TRUNCATED` | header shorter than 512 bytes, or a payload offset+size past EOF |

The safety window (GBATEK Main-RAM limits) defaults, via
`nds_load_bounds_default()`, to:

* Main RAM `0x02000000..0x023BFE00` (legacy DS) or
  `0x02000000..0x02FE0000` (16 MiB TWL);
* ARM7-private WRAM `0x037F8000..0x0380FFC0` (excludes the ARM7 stacks);
* bootloader reservation `0x02100000..0x02280000` — the ARM9 image is linked at
  `0x02200000` and the ARM7 bounce buffer sits at `0x02100000`.

The reservation is deliberately chosen so the canonical homebrew load addresses
(ARM9 `0x02000000`, ARM7 `0x02380000`) stay valid while a malicious header
cannot make the loader overwrite its own running code, `.bss`, stack or the
ARM7 loader before the handoff.

### 25.2 Bounded cluster-chain reader (`src/common/fat_ro.c`)

`fat_read_file_range(vol, entry, offset, dest, len, &got, &status)` streams a
byte range of a file without buffering the whole image. It is hardened against
corrupt FAT chains:

* the walk is capped at `file_size / bytes_per_cluster + 2` hops;
* `0x0000`/`0x0001` (free), `0xFFF7`/`0x0FFFFFF7` (bad) and out-of-range
  clusters are rejected (`FAT_CHAIN_ERR_BAD_CLUSTER`);
* a self-referential cluster or a two-cluster ping-pong is `FAT_CHAIN_ERR_LOOP`;
* an end-of-chain marker before `len` bytes are produced is
  `FAT_CHAIN_ERR_TRUNCATED`.

`fat_read_file()` is now a thin wrapper over it.

### 25.3 Staging + handoff (`src/arm9/loader.c`, `src/arm7/ipc.c`)

1. `loader_validate_file()` reads the header and runs the safety gate.
2. `loader_stage_and_boot()` streams the ARM9 payload straight into its
   validated load address and streams the ARM7 payload into the shared bounce
   buffer at `0x02100000`.
3. The 512-byte header is copied to the canonical firmware address
   (`0x027FFE00` on DS, `0x02FFFE00` on a 16 MiB DSi); the ARM9 D/I caches are
   flushed.
4. `IPC_CMD_BOOT_HANDOFF` (system channel, 4 argument words:
   `arm7_entry, arm7_src, arm7_dst, arm7_size`) asks the ARM7 to finish.
   `arm7_boot_poll()` runs from IWRAM (so it survives a copy that overwrites the
   ARM7's own Main-RAM image at `0x02380000`): it stops all IRQ sources, timers
   and DMA channels, copies the payload to its final address and branches.

The actual CPU handoff is compiled in only when `ENABLE_CHAINLOAD_BOOT` is
defined (the project conventions sandbox-first policy). By default `loader_stage_and_boot()`
performs the entire staging + IPC preparation and returns with
`result.dry_run` set, so melonDS/host diagnostics can exercise the pipeline
without jumping into arbitrary RAM.

### 25.4 UI

In `[6]`:

* **UP/DOWN** move the selection;
* **Y** or **SELECT** run a *Verify / dry-run inspect*: the top screen shows the
  title/game code, unit code, ARM9 `entry/load/size`, ARM7 `entry/load/size` and
  `LOADABLE: YES` or the specific rejection reason;
* **A** stages a verified image and triggers the handoff (`STAGE: DRY-RUN OK`
  without the boot guard, `BOOTING...` with it);
* **B** returns to the menu.

### 25.5 Verification

`tests/test_loader.c` covers valid/bad-CRC headers, ARM9/ARM7 out-of-bounds and
bootloader-overlap ranges, ARM7 WRAM targets, truncated files, the default
bounds, `fat_read_file_range()` across fragmented FAT16 and FAT32 chains
(including non-sector-aligned offsets, multi-sector clusters and EOF clamping)
and the corrupt-chain guards (self-loop/ping-pong/bad/free/premature-EOC/IO).
The 4-byte alignment of the IWRAM load address (required by the crt0 relocation
loop) is enforced by `ld/arm7.ld`/`ld/arm9.ld` with `ALIGN(...)`.

### 25.6 Known limitation

An ARM7 target in ARM7-private WRAM `0x03800000..0x0380FFC0` overlaps the IWRAM
boot stub and stacks that perform the handoff; only Main-RAM ARM7 targets
(e.g. `0x02380000`) are exercised end to end. The boot guard is off by default,
so this path is inert unless explicitly enabled.

## 26. Deterministic SD image builder + chainload test payload

`make sd-image` builds two artefacts that let the whole read-only chainload
pipeline be demonstrated end to end in melonDS and checked in CI:

* `build/hello_payload.nds` — a tiny, self-contained ARM9/ARM7 payload; and
* `build/sdcard.img` — a deterministic 16 MiB MBR + FAT16 SD-card image
  containing `HELLO.NDS`, `BADCRC.NDS`, `OVERLAP.NDS` and `README.TXT`.

### 26.1 The chainload test payload (`payload/`)

`payload/arm9_payload.s` / `payload/arm7_payload.s` are assembled and linked by
`payload/arm9.ld` / `payload/arm7.ld` and packed with `scripts/pack_nds.py`:

| Field        | Value        | Why |
| :----------- | :----------- | :-- |
| ARM9 load/entry | `0x02004000` | inside Main RAM, well below the loader reservation `0x02100000..0x02280000` |
| ARM7 load/entry | `0x02380000` | canonical homebrew ARM7 address (Main RAM) |
| title        | `CHAINLOAD OK` | 12 bytes |
| game code    | `TSTL`       | |
| unit code    | `0x02`       | NDS + DSi hybrid, so melonDS keeps TWL mode |

The ARM9 stub clears any leftover master-brightness fade, paints Engine A's
backdrop bright green (`BGR555 0x03E0`) and Engine B's bright blue
(`0x7C00`), enables normal 2D display (`DISPCNT` bits 16-17 = 1, forced blank
clear) and then idles in a VCOUNT-synchronised loop. The ARM7 stub is a bare
loop. There is no crt0, no stack and no MMIO outside the documented GBATEK map.

`pack_nds.py` gained `--title`, `--gamecode`, `--maker` and `--banner-title`
options. The OpenLaunch defaults are title `OPENLAUNCH` (game code `HNAA`) and
banner title `OpenLaunch\nClean-Room TWL Bootloader`.

### 26.2 The FAT16 image (`scripts/make_sd_image.py`)

Pure Python, no `sudo`/`mount`/`mtools`; the MBR, BPB, FAT tables, root
directory and data clusters are written directly and deterministically (fixed
geometry, fixed volume serial, no timestamps):

| Item              | Value |
| :---------------- | :---- |
| image size        | 16 MiB (32768 sectors) |
| MBR partition 1   | type `0x06` (FAT16), LBA 64, 32704 sectors |
| bytes/sector      | 512 |
| sectors/cluster   | 4 (2 KiB) |
| FATs              | 2 |
| root entries      | 512 (32 sectors) |

Files:

| File          | Content |
| :------------ | :------ |
| `HELLO.NDS`   | the payload, 32 KiB across a **fragmented** 16-cluster chain (every other cluster) so `fat_read_file_range()` exercises the multi-cluster walk |
| `BADCRC.NDS`  | synthetic `.nds` with a deliberately wrong stored header CRC16 (`NDS_ERR_BAD_CRC`) |
| `OVERLAP.NDS` | synthetic `.nds` with a valid CRC but an ARM9 load address of `0x02200000` (`NDS_ERR_OVERLAP_BOOTLOADER`) |
| `README.TXT`  | a non-`.nds` file the browser filter must hide |

### 26.3 Host integration test (`tests/test_sd_image.c`)

`make host-test` first builds `build/sdcard.img`, then `tests/test_sd_image.c`
loads it and drives the real production functions in order:

```
fat_mbr_find_fat_partition() -> fat_mount() -> fat_dir_iter_*()
    -> loader_validate_file() -> loader_stage_and_boot() (dry run)
```

asserting that `HELLO.NDS` validates as `NDS_LOAD_OK` and stages cleanly (both
payloads copied, one `IPC_CMD_BOOT_HANDOFF` request), that `BADCRC.NDS` is
rejected with `NDS_ERR_BAD_CRC`, that `OVERLAP.NDS` is rejected with
`NDS_ERR_OVERLAP_BOOTLOADER`, and that `README.TXT` is hidden by the
`.nds`/`.srl` filter. It also streams the whole `HELLO.NDS` back through the
cluster-chain reader and compares the staged payloads against their ROM offsets.

Because the host cannot write to the fixed load addresses or reach an IPC peer,
`src/arm9/loader.c` is compiled with `-DLOADER_HOST_TEST` for this test only;
that redirects the absolute-address writes into a scratch arena and reports the
handoff request instead of touching hardware. The target build never defines the
macro, so the production path is unchanged.

`scripts/verify_sd_image.py` independently re-checks the generated image's MBR,
BPB, root entries, FAT chains and the two negative-test headers, and is wired
into `make verify` together with a `verify_rom.py` pass over
`build/hello_payload.nds`.

### 26.4 Running it in melonDS

Build the ROM and image (optionally with the live handoff enabled):

```
make clean
make rom sd-image
make CHAINLOAD_BOOT=1 rom sd-image     # arms the real dual-CPU branch
```

`make run` (or `./scripts/run_melonds.sh`) launches the native **`melonDS`**
binary and, before booting, calls `scripts/configure_emulator.py` to point the
native configuration (`~/.config/melonDS/melonDS.toml`, honouring
`XDG_CONFIG_HOME`) at `build/sdcard.img` and select DSi mode — no GUI step is
required. The manual steps below are only needed for a different setup.

Mount the image in DSi mode:

* **Config → Emu settings → Console type:** **DSi**.
* **Config → Emu settings → DSi-mode → DSi SD card:** point at
  `build/sdcard.img` (enable the SD card).
* Provide the DSi BIOS (`bios9i.bin`, `bios7i.bin`, `bios9.bin`, `bios7.bin`)
  and a matching NAND/firmware, or use melonDS' direct-boot path.

Then use menu item `[6] Browse SD Card (.nds)`, select `HELLO.NDS`, press `Y`
to verify (`LOADABLE: YES`) and `A` to stage. Without
`ENABLE_CHAINLOAD_BOOT` the UI reports `STAGE: DRY-RUN OK`; with it, the payload
takes over and the screens turn **green (top)** / **blue (bottom)**, which is
the unmistakable handoff proof. Selecting `BADCRC.NDS` or `OVERLAP.NDS` shows
the specific rejection reason instead.

## 27. Homebrew `argv`, TWL `ARM9i`/`ARM7i` staging and `BOOT.NDS` autoboot

This section covers the three capabilities that turn the chainloader into a
general clean-room Unlaunch alternative: the standard launch `argv` protocol,
optional DSi extended-binary staging and the Unlaunch-style `BOOT.NDS`
autoboot + hotkey policy.

### 27.1 Homebrew `argv` / `__system_argv` protocol

A loaded devkitPro/BlocksDS homebrew learns its launch path from the canonical
`__system_argv` structure. `src/arm9/loader.c` now populates it right after the
512-byte NDS header is copied to its fixed RAM address:

| Region | DS mode | TWL mode |
| :--- | :--- | :--- |
| 512-byte header copy | `0x027FFE00` | `0x02FFFE00` |
| `__system_argv` header (`base + 0x70`) | `0x027FFE70` | `0x02FFFE70` |
| command-line buffer | `0x027FF000` | `0x02FFF000` |

The 24-byte header (little-endian `u32` fields) is:

| Offset | Field | Value |
| :--- | :--- | :--- |
| `+0x00` | `argvMagic` | `0x5F617267` (`"_arg"`) |
| `+0x04` | `commandLine` | command-line RAM pointer |
| `+0x08` | `length` | command-line length, excluding NUL |
| `+0x0C` | `argc` | `1` |
| `+0x10` | `argv[0]` | command-line RAM pointer |
| `+0x14` | `endARGV` | `0xFFFFFFFF` |

The command line is formatted as `"sd:/<8.3 NAME>\0"` — for example
`"sd:/HELLO.NDS"` — by the pure, host-testable `loader_format_command_line()`
and `loader_build_argv()`, and published to high RAM by `loader_stage_argv()`.
All three are bounded: an over-long name or an undersized buffer fails cleanly
and leaves the destination NUL-terminated.

### 27.2 TWL extended binaries (`ARM9i` / `ARM7i`)

`nds_header_t` / `nds_header_parse()` decode the DSi extended header when the
caller supplies at least `NDS_HEADER_TWL_MIN_LEN` (`0x1E0`) readable bytes and
the unit code advertises DSi support (`unit_code & 0x02`):

| Offset | Field |
| :--- | :--- |
| `0x1BF` | `twl_flags` (application flags) |
| `0x1C0` / `0x1C8` / `0x1CC` | ARM9i ROM offset / RAM address / size |
| `0x1D0` / `0x1D8` / `0x1DC` | ARM7i ROM offset / RAM address / size |

`loader_validate_file()` reads a bounded 2-sector (1024-byte) header so those
descriptors are always available, then runs
`nds_header_validate_twl_segments()`. Every non-zero segment must be 4-byte
aligned (offset, address and size), fit inside the file, land in TWL Main RAM
`0x02000000..0x02FE0000` or New Shared WRAM `0x03000000..0x03800000`, and avoid
the bootloader reservation. In TWL mode `loader_stage_and_boot()` streams each
non-zero `ARM9i`/`ARM7i` segment to its validated RAM address alongside the two
primary payloads. A plain NDS image (unit code `0x00`) has no extended segments
and is unaffected.

### 27.3 Unlaunch-style `BOOT.NDS` autoboot + hotkey policy

`boot_resolve_policy()` (pure; `src/arm9/boot.c`) reads the raw, active-low
`KEYINPUT` word captured at startup and resolves:

| Held at boot | Policy |
| :--- | :--- |
| **B** or **SELECT** | `BOOT_POLICY_MENU` — force the interactive diagnostic menu |
| **A**, or nothing | `BOOT_POLICY_AUTOBOOT` — probe the SD root |

`menu_run()` then probes the mounted SD root for `BOOT.NDS` (falling back to
`HELLO.NDS` for the deterministic test image) and reports the outcome on the
top screen as `BOOT : BOOT.NDS READY`, `BOOT : NO BOOT.NDS` or
`BOOT : MENU (KEY HELD)`. Menu item `[8] Autoboot BOOT.NDS` locates, validates
and stages the discovered image through the normal chainload pipeline. When the
ROM is built with `make AUTOBOOT=1` (`-DENABLE_AUTOBOOT`) a discovered
`BOOT.NDS` is booted automatically instead of waiting in the menu.

### 27.4 Deterministic SD image

`scripts/make_sd_image.py` now also installs `BOOT.NDS` — a retitled,
CRC-fixed variant of the `HELLO.NDS` payload (title `AUTOBOOT OK`, game code
`TSTL`) in a fragmented cluster chain. `scripts/verify_sd_image.py` re-checks
its presence, chain, header CRC16 and title.

### 27.5 Verification

* `tests/test_loader.c` checks the `argv` byte layout (magic, pointers, length,
  `argc`, terminator), the `"sd:/HELLO.NDS"` path formatting and the bounds
  failures, plus `nds_header_validate_twl_segments()` for valid, out-of-bounds,
  unaligned, truncated and bootloader-overlapping TWL segments.
* `tests/test_sd_image.c` discovers `BOOT.NDS` on `build/sdcard.img`, validates
  and stages it, and asserts the published `"sd:/HELLO.NDS"` /
  `"sd:/BOOT.NDS"` command lines and the `__system_argv` header.
* `tests/test_menu.c` covers `boot_resolve_policy()` for `KEY_B`, `KEY_SELECT`,
  `KEY_A` and the no-key case.

## 28. Unlaunch-style `BOOT.CFG` hotkey map + one-level `/NDS` subdirectory

This section covers the two usability features that round out the autoboot
path: a deterministic, zero-allocation `BOOT.CFG` text parser that binds the
buttons held at startup to specific images, and a one-level subdirectory
(`/NDS/<FILE>.NDS`) resolution path in both the FAT driver and the browser.

### 28.1 One-level subdirectory support (`src/common/fat_ro.c`)

`fat_file_entry_t` gained an `is_dir` flag decoded from FAT attribute bit `0x10`
(`FAT_ATTR_DIRECTORY`). Two new entry points expose it:

| Function | Behaviour |
| :--- | :--- |
| `fat_list_dir_cluster(vol, start_cluster, entries, max_entries, &count)` | Bounded listing of a directory's cluster chain. `start_cluster == 0` selects the root (the FAT16 fixed area or the FAT32 root cluster chain). Reports files *and* directories; skips LFN, volume-label and deleted entries. |
| `fat_find_path(vol, path, &entry)` | Resolves a root 8.3 name (`"BOOT.NDS"`, `"/BOOT.NDS"`) or a one-level path (`"NDS/HELLO.NDS"`, `"/NDS/HELLO.NDS"`), matching both components case-insensitively. |

`fat_list_dir_cluster()` is hardened exactly like `fat_read_file_range()`: the
chain walk is capped at `cluster_count + 2` hops, a self-loop or two-cluster
ping-pong is reported as `FAT_DIR_ERR_LOOP`, free/bad/out-of-range clusters as
`FAT_DIR_ERR_BAD_CLUSTER`, and a failed sector read as `FAT_DIR_ERR_IO`.
`FAT_DIR_OK == 0`, so a truthy return value always means "not OK".

`fat_find_path()` fails closed on a missing directory, a file used as a
directory (`"HELLO.NDS/SUBAPP.NDS"`), a path deeper than one level
(`"A/B/C.NDS"`) and empty components (a trailing or doubled slash). The
existing `fat_dir_next()` iterator still hides subdirectories, so the root
browser list is unchanged.

`loader_format_command_line()` normalises a leading slash away (because
`"sd:/"` is always prepended), so a subdirectory target formats as
`"sd:/NDS/HELLO.NDS\0"` while a root file stays `"sd:/BOOT.NDS\0"`.
`loader_stage_and_boot_path()` publishes that exact string through
`__system_argv` (see §27.1).

### 28.2 The `BOOT.CFG` hotkey map (`src/arm9/boot_cfg.c`)

`BOOT.CFG` is an optional, read-only text file in the SD root:

```
# OpenLaunch autoboot configuration
DEFAULT=BOOT.NDS
A=HELLO.NDS
X=NDS/SUBAPP.NDS
```

Supported keys are `DEFAULT`, `A`, `X`, `Y`, `L`, `R` and `START`. `boot_cfg_parse()`
is a pure, bounds-checked pass over a single 512-byte sector:

* it resets `out` to the built-in defaults (`DEFAULT = BOOT.NDS`,
  `A = HELLO.NDS`, `X = NDS/HELLO.NDS`, `Y`/`L`/`R`/`START` unbound) and then
  applies every recognised `KEY=PATH` line (last duplicate wins);
* blank lines, whole-line and trailing `#` comments, CR/LF/CRLF endings and a
  final line without a newline are all accepted;
* malformed lines (no `=`), unknown keys, invalid path characters, empty
  values and over-long targets are dropped without disturbing the rest of the
  map and without ever reading past the buffer;
* **`B` and `SELECT` are not recognised keys**: they can never be overridden
  and always force the interactive diagnostic menu.

`boot_resolve_target(held_keys, cfg, &out)` is the pure resolver. `held_keys`
is the active-high logical key mask (`input_state_t.held`: `KEYINPUT` merged
with the ARM7-forwarded `EXTKEYIN` bits, so `X`/`Y` are visible once
`input_update_ext()` has run). `B`/`SELECT` yield `BOOT_POLICY_MENU`; otherwise
the held mapped button is looked up in the fixed priority order `A`, `X`, `Y`,
`L`, `R`, `START`, and failing that the `DEFAULT` target is used. An unbound
button falls back to `DEFAULT` with `has_key == false`.

### 28.3 Boot-flow integration (`src/arm9/menu.c`)

On startup `menu_load_boot_cfg()` mounts the SD volume, reads up to one sector
of `BOOT.CFG` through the delegated FAT reader (`fat_find_path()` +
`fat_read_file_range()`; the read buffer lives in `.bss`, i.e. shared Main
RAM) and parses it. With no `BOOT.CFG` the built-in defaults apply. The menu
then polls `EXTKEYIN` once, resolves the target from the held buttons and
probes the resolved path.

The top-screen telemetry line shows the resolution:

| Situation | `BOOT :` line |
| :--- | :--- |
| `B`/`SELECT` held | `BOOT : MENU (KEY HELD)` |
| config present, no hotkey | `BOOT : BOOT.NDS [CFG]` |
| `X` held (mapped) | `BOOT : NDS/HELLO.NDS [X]` |
| target found by fallback | `BOOT : HELLO.NDS` |
| not found | `BOOT : NO BOOT.NDS` |

Menu item `[6] Browse SD Card (.nds)` now lists the root `.nds`/`.srl` files
**and** the `.nds`/`.srl` files inside `/NDS/`, the latter prefixed as
`NDS/<NAME>`. Item `[8] Autoboot BOOT.CFG` stages the resolved target through
the normal chainloader, publishing the correct (possibly subdirectory)
`__system_argv` path.

### 28.4 Deterministic SD image

`scripts/make_sd_image.py` now also creates:

* a `/NDS` subdirectory containing `NDS/SUBAPP.NDS` — a retitled, CRC-fixed
  copy of the payload. The NDS title field is exactly 12 bytes, so
  `SUBDIR PAYLOAD` is stored as `SUBDIR PAYLO`;
* a root `BOOT.CFG` (CRLF endings) mapping `DEFAULT=BOOT.NDS`, `A=HELLO.NDS`
  and `X=NDS/SUBAPP.NDS`.

`scripts/verify_sd_image.py` independently re-checks the subdirectory entry,
the `NDS/SUBAPP.NDS` chain/CRC/title and the `BOOT.CFG` contents.

### 28.5 Verification

* `tests/test_fat_ro.c` grew `fat_list_dir_cluster()` (root + subdirectory,
  `max_entries` capping, self-loop and bad-cluster guards) and `fat_find_path()`
  (root/leading-slash/case-insensitive, one-level subdirectory, missing
  directory, file-as-directory, over-deep and malformed paths) coverage.
* `tests/test_menu.c` covers `boot_cfg_parse()` (LF/CRLF, comments, every
  supported key, truncated prefixes, unknown keys, `B`/`SELECT` immunity,
  invalid characters, over-long targets, whitespace, duplicates) and
  `boot_resolve_target()` (default, A/X/Y, priority, menu override, parsed
  config, NULL handling).
* `tests/test_loader.c` checks the `"sd:/NDS/HELLO.NDS"` path formatting and
  the leading-slash normalisation.
* `tests/test_sd_image.c` resolves `NDS/SUBAPP.NDS` on `build/sdcard.img`,
  validates and stages it, and asserts the published `"sd:/NDS/SUBAPP.NDS"`
  command line, plus the parsed `BOOT.CFG` map.

## 29. DLDI section scanner / in-RAM patcher + `[9] Inspect BOOT.CFG`

This section covers the two homebrew-compatibility and diagnostic-visibility
features: a clean-room DLDI (Dynamically Linked Device Interface) scanner and
optional in-RAM patcher, and a dedicated `BOOT.CFG` / hotkey inspector.

### 29.1 The DLDI section (`include/common/dldi.h`, `src/common/dldi.c`)

DLDI is Chishm's position-independent storage-driver format used by libfat. A
homebrew `.nds` reserves a block of RAM for a "DLDI section" and marks it with a
fixed 12-byte signature; a loader replaces that block with a real device driver.
The clean-room module understands only the on-disk/on-RAM structure — it never
touches hardware.

Canonical header (GBATEK "DS Cart DLDI Driver"):

| Offset | Size | Field |
| :--- | :--- | :--- |
| `0x00` | 4 | magic `0xBF8DA5ED` |
| `0x04` | 8 | `" Chishm\0"` signature string |
| `0x0C` | 1 | `version` (1) |
| `0x0D` | 1 | `driver_size_log2` (log2 of `.dldi` + BSS) |
| `0x0E` | 1 | `fix_flags` (`DLDI_FIX_*`) |
| `0x0F` | 1 | `allocated_size_log2` (log2 of RAM reserved in the `.nds`) |
| `0x10` | 48 | NUL-padded friendly name |
| `0x40` | 32 | address table: `data_start/data_end`, `glue_start/glue_end`, `got_start/got_end`, `bss_start/bss_end` |
| `0x60` | 4 | IOType tag (`"NONE"`, `"DSiR"`, …) |
| `0x64` | 4 | `features` (`FEATURE_*`) |
| `0x68` | 24 | 6 `IO_INTERFACE` function pointers (`startup`, `is_inserted`, `read_sectors`, `write_sectors`, `clear_status`, `shutdown`) |
| `0x80` | .. | driver code |

The pure, bounds-checked API:

| Function | Behaviour |
| :--- | :--- |
| `dldi_find_offset(buf, len, &off)` | Scan `buf` on 4-byte boundaries for the 12-byte signature, requiring room for a whole header after the match. |
| `dldi_parse_header(buf, len, &info)` | Decode the header; require `allocated_size_log2` in `[10, 16]` (1 KiB..64 KiB) and `1 << allocated_size_log2 <= len` (for a target stub, `len` is the remaining ARM9 image after the stub). Strings are NUL-terminated and sanitised. |
| `dldi_scan(buf, len, &info, &off)` | `dldi_find_offset()` + `dldi_parse_header()` in one call. |
| `dldi_apply_patch(...)` | Validate both headers, require `driver_size_log2 <= allocated_size_log2` and that the driver fits, copy the driver over the stub and relocate. |

`dldi_apply_patch(arm9_buf, arm9_len, arm9_ram_base, driver_buf, driver_len)`
mirrors the documented `dlditool` algorithm. The stub header begins at
`arm9_buf + off`, which is loaded at `arm9_ram_base + off`; the delta is
`(arm9_ram_base + off) - driver.data_start`. After the copy the eight address
fields and six function pointers are shifted by the delta, then:

* `DLDI_FIX_ALL` (`0x01`) walks `[data_start, data_end)` and shifts every
  32-bit word that still points into the driver's original window;
* `DLDI_FIX_GLUE` (`0x02`) does the same for `[glue_start, glue_end)`;
* `DLDI_FIX_GOT` (`0x04`) for `[got_start, got_end)`;
* `DLDI_FIX_BSS` (`0x08`) zero-fills `[bss_start, bss_end)`.

Header fields are rebased first, so they fall outside the driver window and the
range guard skips them (no double relocation), exactly as the reference tool
intends. The target's `allocated_size_log2` is preserved in the patched header.

`dldi_builtin_ro_driver()` returns a 1 KiB clean-room, position-independent
read-only driver template: IOType `"DSiR"`, `driver_size_log2 =
allocated_size_log2 = 10`, `FIX_BSS`, and six in-image ARM stubs (`startup`,
`is_inserted`, `clear_status`, `shutdown` return true; `read_sectors` and
`write_sectors` fail safely, so no medium is ever written).

### 29.2 Boot-flow integration (`src/arm9/loader.c`, `src/arm9/menu.c`)

`loader_result_t` gained `dldi_present`, `dldi_patched`, `dldi_alloc_kb` and the
4-char `dldi_io_type`. During `loader_stage_and_boot()` the staged ARM9 segment
is scanned (`dldi_scan`); a valid stub is reported, and when its IOType is the
default `"NONE"` placeholder and space permits it is patched in RAM with the
built-in driver before the cache flush / IPC handoff. Item `[6]`'s `Y`/`SELECT`
verify view uses `loader_detect_dldi()` (a bounded prefix read into a 16 KiB
`.bss` window) and renders `DLDI : YES (<io_type>, <alloc_kb>K)` or
`DLDI : NONE`.

The deterministic payload (`payload/arm9_payload.s` + `payload/arm9.ld`) now
begins with a 1 KiB, 1 KiB-aligned `.dldi` section carrying the canonical
`"NONE"` stub, so `HELLO.NDS` / `BOOT.NDS` / `NDS/SUBAPP.NDS` exercise real
detection and patching on `build/sdcard.img`.

### 29.3 `[9] Inspect BOOT.CFG`

Menu item `[9]` (`MENU_ACTION_INSPECT_CFG`) renders a read-only screen:

```
BOOT.CFG / HOTKEY INSPECT
SOURCE : SD:/BOOT.CFG        (or DEFAULTS)
SD     : MOUNTED             (or UNAVAILABLE)
BINDING / TARGET
DEFAULT: BOOT.NDS [OK]
A      : HELLO.NDS [OK]
X      : NDS/SUBAPP.NDS [OK]
Y      : -
L      : -
R      : -
START  : -
B/SEL  : FORCE MENU (LOCKED)
B: RETURN
```

The source line reflects whether a `BOOT.CFG` was loaded or the built-in
defaults apply; each recognised binding shows its configured path and a live SD
target check (`OK` / `MISSING`); unbound keys render `-`; and the unbrickable
`B`/`SELECT` override is shown as locked. `B` returns to the menu.

### 29.4 Verification

* `tests/test_dldi.c` (suite 10) covers `dldi_find_offset()` (present/absent,
  4-byte alignment, header-room rejection), `dldi_parse_header()` (field decode,
  the 10..16 bounds, the fit check, string sanitisation), `dldi_apply_patch()`
  (address-table and function-pointer relocation, the `FIX_ALL` / `FIX_GLUE` /
  `FIX_GOT` pointer walks, `FIX_BSS` zeroing, and the
  `driver_size_log2 > allocated_size_log2` rejection) and the built-in `"DSiR"`
  template.
* `tests/test_menu.c` asserts the nine-item action map (including
  `MENU_ACTION_INSPECT_CFG`).
* `tests/test_sd_image.c` asserts `HELLO.NDS` / `BOOT.NDS` /
  `NDS/SUBAPP.NDS` report `dldi_present` and `dldi_patched` (`"DSiR"`, 1 KiB)
  after staging, and that only the replaced DLDI block differs from the on-disk
  ARM9 bytes.
* `scripts/verify_sd_image.py` independently finds the unpatched `"NONE"` stub,
  checks `allocated_size_log2 == 10` and that the reserved block fits the ARM9
  segment for all three payloads.

## 30. NDS banner (`0x068`) decoder + reproducible release packaging

This section covers two release-readiness features: a pure NDS banner parser
that decodes the English title for the `[6]` file inspector, and a
`make release` bundle that stages every production variant.

### 30.1 The banner block

An `.nds` image points at its banner with the 32-bit word at header offset
`0x068`; the block itself is `0x840` (2112) bytes and is normally placed at file
offset `0x840` (inside the `0x4000`-byte header region, before the ARM9 binary).
Layout of the fields the reader touches:

| Offset | Size | Field |
| :--- | :--- | :--- |
| `0x000` | 2 | version (`0x0001`/`0x0002`/`0x0003`/`0x0103`) |
| `0x002` | 2 | CRC-16/MODBUS over `0x020..0x83F` (`0x820` bytes) |
| `0x020` | `0x220` | 32x32 4bpp icon bitmap + 32-byte palette |
| `0x240` | 256 | primary/English title, UTF-16LE, 128 code units |

`src/common/nds_header.c` exposes `nds_banner_parse(buf, len, out)`, a pure,
bounds-checked reader. It:

* requires `len >= 0x840` and checks the version against the accepted set
  (`version_ok`);
* recomputes the Modbus/ARC CRC-16 (seed `0xFFFF`, polynomial `0xA001`) over
  `0x020..0x83F` and compares it with the stored value (`crc_ok`);
* converts the UTF-16LE title at `0x240` into a compact, NUL-terminated ASCII
  summary: printable ASCII (`0x20..0x7E`) verbatim, a line break (`0x000A`, or a
  `0x000D`/`0x000A` CRLF pair collapsed) as the 3-character separator `" / "`,
  and every other code unit as `'?'`. Trailing spaces are trimmed.

`nds_header_parse()` additionally decodes the `banner_offset` field into
`nds_header_t.banner_offset`. The chainloader exposes
`loader_read_banner()` (in `src/arm9/loader.c`), which streams the `0x840`-byte
block through the delegated FAT reader into a fixed `.bss` window and runs the
parser; the `[6]` `Y`/`SELECT` inspector rows now render
`BANNER: <title>` / `BANNER: BAD CRC` / `BANNER: NONE`.

`scripts/pack_nds.py` writes a valid v1 banner at `0x840` with a correct CRC16
and a `--banner-title`-customisable UTF-16LE title (defaulting to the ROM title;
a literal `\n` becomes `" / "` when decoded). `scripts/make_sd_image.py`
retitles the banner in step with the `BOOT.NDS`/`NDS/SUBAPP.NDS` variants and
recomputes its CRC16.

### 30.2 Reproducible release bundle (`make release`)

`scripts/make_release.py` (driven by `make release`) builds each production
variant in its own isolated `build/_release_work/<key>` directory — so the
default `build/` tree is untouched — and stages `build/release/`:

| Artifact | Build |
| :--- | :--- |
| `openlaunch_diag.nds` | default (safe dry-run / diagnostic) |
| `openlaunch.nds` | `CHAINLOAD_BOOT=1` (live chainload on `A` / `[8]`) |
| `BOOT.NDS` | `CHAINLOAD_BOOT=1 AUTOBOOT=1` (drop-in autoboot) |
| `BOOT.CFG` | sample SD-root hotkey map (`DEFAULT=NDS/HELLO.NDS`) |
| `NDS/HELLO.NDS` | the standalone chainload test payload |
| `sdcard.img` | the pre-built 16 MiB FAT16 melonDS test image |
| `SHA256SUMS.txt` | deterministic SHA-256 of every artifact |

The `BOOT.CFG` sample deliberately targets `NDS/HELLO.NDS`, never `BOOT.NDS`
itself (that is the autoboot bootloader), so the drop-in `BOOT.NDS` cannot
recursively boot itself. `SHA256SUMS.txt` is written in sorted order and then
re-read and re-hashed; the script reports the verified digest count before
producing a deterministic `build/openlaunch_release.tar.gz` (zeroed owner
and timestamp metadata, fixed `mtime`).

### 30.3 Verification

* `tests/test_loader.c` covers `nds_banner_parse()`: valid v1/v2/v3/0x0103
  banners, the `"Line 1\nLine 2"` → `"Line 1 / Line 2"` multi-line form, CRLF
  collapsing, non-ASCII `'?'` substitution, trailing-space trimming, bad CRC16
  detection, unsupported versions and NULL/short-buffer rejection, plus the
  `nds_header_t.banner_offset` decode.
* `tests/test_sd_image.c` reads the banner of `HELLO.NDS` (`CHAINLOAD OK`),
  `BOOT.NDS` (`AUTOBOOT OK`) and `NDS/SUBAPP.NDS` (`SUBDIR PAYLOAD`) through
  `loader_read_banner()` and asserts `version_ok` + `crc_ok`; the `BADCRC.NDS`
  stub is asserted to have `banner_offset == 0` and no banner.
* `scripts/verify_rom.py` checks the `0x068` offset, the banner CRC16 and the
  decoded title; `scripts/verify_sd_image.py` does the same for all three
  payloads inside `build/sdcard.img`.

---

## 31. Graphical bitmap font & banner-icon renderer (`src/arm9/ui_font.c`)

The text console (`src/arm9/console.c`) is augmented by a zero-allocation
graphical renderer. Everything lives on **Engine A BG0**, so no extra layer,
window or blend is required.

### 31.1 Engine A BG0 layout

`console_init()` maps bank A (128 KiB) to Engine A BG at `0x06000000`. To leave
room for the overlay tiles the Engine A screen base block was moved to 16 (the
Engine B map is unchanged at block 2):

| Region | Address | Contents |
| :--- | :--- | :--- |
| tiles 0..95 | `0x06000000` | the 8x8 console font (96 ASCII glyphs) |
| tiles 96..479 | `0x06000C00` | the 16x16 title font (4 tiles per glyph) |
| tiles 480..495 | `0x06003C00` | the 32x32 banner icon (4x4 tiles) |
| map 32x32 | `0x06008000` | the text map (screen base block 16) |

`BG0CNT` is therefore `0x1000` on Engine A (`0x0200` on Engine B); the register
is now per-engine in the `console_engine_t` table.

### 31.2 The 16x16 double-size font

`ui_font_init()` expands each 1bpp 8x8 glyph from `font8x8.c` into four 8x8 4bpp
tiles (`ui_font_expand_glyph16()`: 2x2 pixel replication), storing 384 tiles in
VRAM. No second font table is linked, so the ROM stays small. `ui_font_
render_line16()` writes a 2x2 cell block per character with full clipping
against the visible rows/columns and the physical map stride.

### 31.3 The banner icon

The NDS banner block (`0x068`) carries a 32x32 icon at `0x020` and a 16-colour
BGR555 palette at `0x220`. `ui_font_decode_icon()` de-tiles the 4x4 tile grid
row-major into 1024 palette indices; both 4bpp (two pixels/byte, low nibble
first) and 8bpp (one byte/pixel) layouts are supported. The ARM9 draws it with
`ui_font_draw_icon()`, which installs the tiles at 480..495 and the palette in
BG palette **bank 7** (unused by the console); icon palette entry 0 is forced to
black so the thumbnail matches the AMOLED-friendly black backdrop. The
verify/dry-run inspector decodes and shows the icon and the double-size title
when a `.nds` file is highlighted in menu `[6]`. The raw banner block is exposed
by `loader_banner_raw()` so no second FAT read is needed.

### 31.4 Verification

`tests/test_ui_font.c` covers the 4bpp/8bpp tile decoders, the full icon
de-tiler for both depths, the 4bpp pack round-trip, the 16x16 glyph expander
(including the top-left and bottom-right corner pixels), the palette reader, the
UTF-16 title sanitizer (line breaks, non-ASCII, truncation, NULL safety) and the
bounded 16x16 line renderer (right-edge, bottom-row, negative-x and off-screen
clipping).

---

## 32. NAND partition tables & HiyaCFW redirect (`src/common/nand.c`)

### 32.1 Read-only NAND access

The DSi uses a 240 MB Samsung eMMC whose first sector is a PC-style MBR (GBATEK
"DSi SD/MMC Internal NAND Layout"). The eMMC controller is the same TMIO block
as the SD card (port 1), which is reachable only from the ARM7, so every NAND
read is delegated over IPC with `IPC_STORAGE_DEV_EMMC` — exactly like the SD
reader. `src/arm9/nand_boot.c` provides `arm9_nand_read_sectors()`,
`arm9_nand_probe()`, `arm9_nand_mount_fat()` and `arm9_nand_find_launcher()`.

No NAND write command exists: the module is strictly read-only, preserving the
anti-brick guard (the project conventions). `IPC_CMD_WRITE_SECTOR` remains unimplemented.

### 32.2 Two partition-table layouts (`src/common/nand.c`)

* **PC-style MBR** (`NAND_TABLE_PC_MBR`): signature `0x55AA` at `0x1FE`, four
  16-byte entries at `0x1BE` with the standard `type` (`+4`), `lba` (`+8`) and
  `sector_count` (`+12`) little-endian fields. FAT16/FAT32 types are `0x04`,
  `0x06`, `0x0B`, `0x0C`, `0x0E`.
* **TWL / HiyaCFW** (`NAND_TABLE_TWL`): the same signature and table offset, but
  each entry stores `{offset, size}` little-endian 32-bit sector numbers at
  `+0`/`+4` and carries no type byte. The first (main) partition is the FAT
  volume; the caller confirms it with a mount.

`nand_parse_table()` auto-detects: a PC MBR with a recognised FAT entry wins,
then a non-empty TWL table, then any PC entries. `nand_find_fat_partition()`
and `nand_find_largest_fat()` pick the first / largest FAT volume. The deep
resolver `nand_resolve_path()` walks an absolute FAT path component by
component (case-insensitive, bounded by `NAND_PATH_MAX_DEPTH`).

### 32.3 Boot redirect policy (`boot_resolve_nand_target()`)

The pure policy mirrors Unlaunch/HiyaCFW precedence:

1. **B/SELECT held** → keep the interactive menu (`BOOT_POLICY_MENU`).
2. **SD target found** (`BOOT.NDS` / a `BOOT.CFG` binding) → boot from SD.
3. **No SD target, NAND FAT volume present** → redirect to the DSi System Menu.
4. Otherwise → nothing to boot.

The DSi System Menu is TWL title `00030004`-`484E4C41` ("HNLA"); the launcher is
`/title/00030004/484e4c41/content/00000000.app`. HiyaCFW's redirected root is
searched as `/rootfs/title/00030004/484e4c41/content/00000000.app`. The menu
`[8] Autoboot BOOT.CFG` action attempts the redirect when no SD target is found
and reports `NAND System Menu located` (the launcher is only located and
reported — never written — so the anti-brick guard is preserved).

The top-screen telemetry gains a `NAND : DETECTED` / `NAND : ABSENT` row (the
SD capacity readout moved to row 17).

### 32.4 Verification

`tests/test_nand.c` synthesizes PC-style MBR and TWL `{offset,size}` sectors and
asserts the decoded tables (counts, FAT classification, largest partition, total
size, auto-detection and signature/NULL rejection), the boot-redirect policy for
every input combination, and the deep FAT resolver against
`build/sdcard.img`.

---

## 33. Boot theme engine, accent colours & SD splash (`src/arm9/theme.c`)

The bootloader keeps its AMOLED-friendly black background but the palette is now
driven from `BOOT.CFG` by a small, zero-allocation theme engine
(`include/arm9/theme.h`, `src/arm9/theme.c`). The engine attributes are parsed
by the same single-sector `boot_cfg_parse()` pass as the hotkey map, so no
second read is required.

### 33.1 `BOOT.CFG` theme directives

| Directive | Value | Meaning |
| :--- | :--- | :--- |
| `THEME_ACCENT` | RGB888 | selected borders, active tabs, telemetry headers |
| `THEME_TEXT` | RGB888 | standard console text |
| `THEME_BG` | RGB888 | backdrop and window frames |
| `SPLASH_IMAGE` | path | raw 256x192 15-bit BGR555 bitmap on the SD root |

Colours accept `#RRGGBB`, `0xRRGGBB`, a bare `RRGGBB` or the `#RGB` shorthand.
A `#` starts a comment only at the start of a line or after whitespace, so a
`#RRGGBB` colour value is never mistaken for a comment. Omitted or malformed
directives keep the defaults:

```
ACCENT = 0x00FF88   TEXT = 0xFFFFFF   BG = 0x000000
```

### 33.2 Colour model

`theme_parse_color()` decodes the RGB888 token and `theme_rgb_to_bgr555()`
converts it to the hardware BGR555 palette word
(`((r>>3)<<0) | ((g>>3)<<5) | ((b>>3)<<10)`). `theme_apply()` recolours the
Engine A/B console palettes: BANK 0 (white) becomes the text colour, BANK 1
(cyan, used by every telemetry/menu header) and BANK 3 (yellow, used by the
selected row) become the accent, and palette entry 0 (the backdrop) becomes the
background colour. The banner-icon palettes (bank 7) are left untouched.

### 33.3 Splash image format

A valid splash is a raw `SPLASH.RAW` (or `.BIN`) file of exactly
`256 * 192 * 2 = 98304` bytes: little-endian 15-bit BGR555 pixels, row-major,
no header. `theme_splash_check()` fails closed on any other size or extension
(indexed `.BMP` is intentionally rejected). On the DS the upper bit of a direct
colour bitmap pixel is the alpha flag (`0 = transparent, 1 = opaque`), so
`theme_splash_blit()` ORs `0x8000` into every pixel as it reaches VRAM; the
on-disk format stays a plain 15-bit bitmap.

### 33.4 Engine A BG2 presentation

`theme_load_splash()` streams the validated file through the delegated FAT
reader into a static 96 KiB staging buffer and `theme_present_splash()` blits it
to Engine A BG2 VRAM:

| Item | Value |
| :--- | :--- |
| VRAM bank | B, mapped as Engine A BG at offset 1 (`VRAMCNT_B = 0x89`) |
| framebuffer | `0x06020000` (exactly 65536 x 16-bit pixels) |
| BG2CNT | `0x4887` = 16bpp extended affine, 256x256, bitmap base 8, priority 3 |
| DISPCNT | Engine A **mode 5** (`BG0/BG1` text + `BG2/BG3` extended bitmap) |

BG0 keeps priority 0, so the text console and the banner overlay draw on top of
the bitmap: the splash becomes the backdrop of the whole menu. When no valid
splash is configured (or the card/file is unavailable) the engine falls back
cleanly to the pure `THEME_BG` backdrop with accent-coloured UI borders.

### 33.5 Zero-allocation staging

`theme_load_splash()` uses one fixed `static u8 s_splash_stage[98304]` in
`.bss` (shared Main RAM, never DTCM) and `fat_read_file_range()` for the bounded,
loop-guarded cluster walk. `theme_splash_read()` is the MMIO-free core and is
exercised directly by the host tests.

### 33.6 Verification

`tests/test_theme.c` (suite 13) covers the RGB888 -> BGR555 conversion and every
colour token form (`#`/`0x`/bare/`#RGB`, malformed and over-long input), the
AMOLED defaults, the `BOOT.CFG` directive extraction alongside the hotkey map
(including `#RRGGBB`-is-not-a-comment and the malformed-directive fallbacks), the
splash validator (extension + exact size), the alpha-forcing blit, and the
staged splash read against the generated `build/sdcard.img`. `make sd-image`
now installs a deterministic gradient `SPLASH.RAW` and a themed `BOOT.CFG`, and
`scripts/verify_sd_image.py` independently re-checks both.

---

## 34. Safe NAND installer (`src/common/installer.c`, `src/arm9/installer_*.c`)

A guarded, sandboxed installer pipeline for the internal DSi eMMC, exposed as
menu item `[10] Install to NAND`. The design follows the project conventions to the letter:
the default build is physically incapable of writing the eMMC, the mandatory
compile-time gate is `-DENABLE_NAND_WRITE_DANGER` (knob: `make NAND_WRITE_DANGER=1`,
isolated tree: `make nand-installer`), and even in an armed build the write path
stays locked until a signed package authenticates **and** a full NAND backup has
been written and verified.

> **Scope (read this first).** `[10]` is an **unencrypted sandbox staging
> harness**, not a production installer. A retail DSi NAND is encrypted with a
> per-console AES-CTR key (keyX from the Console ID, IV from `SHA1(CID)`) and
> its boot chain is RSA/`.tmd` checked, so writing raw plaintext there can never
> install anything and will corrupt or brick the console. The installer now
> classifies the image up front and refuses a retail image with
> `INSTALLER_ERR_ENCRYPTED_NAND` (`"retail encrypted NAND (AES/TMD required)"`).
> On real hardware, run OpenLaunch by SD chainloading (`BOOT.NDS` / the Unlaunch
> file menu) - it never writes NAND. Real-install support needs an AES-CTR +
> decrypted-`twln` + `.tmd` layer that does **not** exist yet (see §34.10).

### 34.1 Layers

| Layer | Files | Compiled | Purpose |
| :--- | :--- | :--- | :--- |
| SHA-256 / HMAC-SHA-256 | `include/common/sha256.h`, `src/common/sha256.c` | always | backup/payload fingerprints and the token MAC |
| Layout + guards | `src/common/nand.c` | always | MBR/TWL validation, protected boot prefix |
| Token + policy + state machine | `include/common/installer.h`, `src/common/installer.c` | always | signed descriptor, pre-flight gate, abort/error vocabulary |
| FAT root-file writer | `include/common/fat_rw.h`, `src/common/fat_rw.c` | always | writes `NANDBACK.BIN` on the SD card |
| Backup / verify / flash engines | `include/arm9/installer.h`, `src/arm9/installer_backup.c`, `src/arm9/installer_install.c` | always (engines), guarded (glue) | streamed, injected-I/O transfers |
| ARM7 write + guard | `src/arm7/ipc.c`, `src/arm7/sdmmc.c` | guarded | the only code that can write the eMMC |

The injected-I/O engines carry no hardware access, so the whole
backup → verify → flash → read-back sequence is host-testable
(`tests/test_installer.c`, suite 14).

### 34.2 SHA-256 / HMAC-SHA-256

`src/common/sha256.c` is a freestanding FIPS 180-4 / RFC 2104 implementation
(no libc, no allocation; the state lives on the stack). `sha256_digest_equal()`
compares in constant time and `sha256_hex()` renders the 64-character
lower-case form used by the on-screen report and `SHA256SUMS` tooling.

### 34.3 NAND geometry + pre-flight layout validation

`nand_validate_layout()` verifies the `0x55AA` signature, auto-detects the
PC-style MBR vs the HiyaCFW TWL `{offset,size}` form, requires a FAT16/FAT32
partition at least `NAND_MIN_FAT_SECTORS` (128 KiB) large, bounds it against
`NAND_MAX_SECTORS` (256 MiB) and records the protected boot prefix
(`protected_end = partition_lba`, at least `NAND_BOOT_GUARD_SECTORS`).
`nand_region_is_protected()` rejects any overlapping target, overflowing
arithmetic fails closed, and these checks run **before** the installer reads a
single payload byte.

`nand_classify_image()` (with `nand_detect_unlaunch_footer()`) classifies the
image from its sector 0 and its byte-`0xFF800` footer: a `no$gba`/Unlaunch
footer (`"DSi eMMC CID/CPU"`), or an MBR whose `0x55AA` signature is absent
because it is still AES-CTR encrypted, marks the image
`NAND_IMAGE_ENCRYPTED_RETAIL`; only a plaintext `0x55AA` MBR with no footer is
`NAND_IMAGE_DECRYPTED_SANDBOX`. The classification is fail-closed (an
unreadable MBR is `NAND_IMAGE_UNKNOWN`) and is fed to the pre-flight gate.

> **Sandbox scope.** The installer never writes the protected prefix that
> physically precedes the user FAT volume (the stage-1/stage-2 bootstrap area).
> The target region must lie inside the user FAT volume; the boot prefix is
> permanently off-limits. This is the project conventions "sandbox-first" posture: the
> installer is a safe in-volume staging mechanism, not a factory-boot overwriter.

### 34.4 Signed installer token + pre-flight state machine

`INSTALL.KEY` (96 bytes) carries the target descriptor and a payload digest,
authenticated by an HMAC-SHA-256 over the first `0x40` bytes (see
include/common/installer.h for the byte-exact layout). `installer_token_parse()`
structurally validates it, `installer_token_verify()` authenticates it, and
`installer_token_check_range()`/`installer_plan_region()` bound the target to
the validated layout and the exact payload size.

`installer_preflight()` is a strict boolean cascade (build flag → SD
destination → layout → token → prior backup); the state machine
`IDLE → PREFLIGHT → BACKUP → VERIFY_BACKUP → UNLOCK → WRITE → VERIFY_WRITE →
DONE`, with any failure moving to `ABORTED`, is `installer_advance()`.

`scripts/make_installer_token.py` produces the token and self-verifies it; it
shares the build's development key (`INSTALLER_AUTH_KEY`, overridable at compile
time). The token is a **runtime** authorisation only — it can never enable
writing in a build that did not define `ENABLE_NAND_WRITE_DANGER`.

### 34.5 Mandatory backup + FAT writer

Before any write, `installer_run()` streams the NAND (from LBA 0 to the
layout's `total_backup_sectors`, or to `INSTALLER_SANDBOX_BACKUP_SECTORS` when
the token carries the sandbox flag) through `installer_backup_stream()` into the
SD root file `NANDBACK.BIN`, hashing with SHA-256. Stage `3/6 VERIFY_BACKUP`
then proves the backup two ways:

1. `installer_backup_verify()` re-reads the **live NAND** and compares digests
   (proves the source was stable); and
2. `installer_backup_verify_sd()` re-opens `NANDBACK.BIN` on the mounted **SD**
   volume (`fat_find_path()` + `fat_read_file_range()`), checks the on-disk size
   and streams the file back through SHA-256, comparing it to the NAND digest.

The second check is the one that matters: a dropped or truncated SD write (for
example a read-only emulated card, which melonDS silently discards) leaves a
missing or short file and is caught deterministically before `4/6 UNLOCK`. A
mismatch, a short write or an I/O error aborts the installation with the
backup's clusters freed.

`src/common/fat_rw.c` is a bounded FAT16/FAT32 root-file writer
(`fat_create_file()` → `fat_writer_write()` → `fat_writer_end()`, plus
`fat_writer_abort()`): only the root directory, only 8.3 names, no overwrite (a
name clash fails closed), cluster/FAT allocation on every FAT copy, and every
step guarded. It exists solely so the backup can be a real file; it is covered
by `tests/test_installer.c` against `build/sdcard.img`.

> The transfer engine streams one 512-byte sector per delegated IPC request.
> This is intentionally small and bounded (it keeps the ARM7 responsive and the
> buffers in shared RAM); batching up to `IPC_STORAGE_MAX_SECTORS` (16 KiB) per
> request is an obvious future optimisation for a full 240 MB NAND dump.

### 34.6 Flash, runtime guard and IPC

`installer_flash_region()` writes each target sector, reads it back
immediately and compares byte-for-byte before advancing; the accumulated digest
is finally checked against the token. The gated glue arms the ARM7 with a
token-derived id (`IPC_CMD_NAND_GUARD`), streams through `IPC_CMD_WRITE_SECTOR`
and disarms unconditionally. The ARM7 refuses `IPC_STORAGE_DEV_EMMC` writes with
`IPC_IO_FLAG_LOCKED` until armed; SD writes (the backup) are allowed whenever the
build flag is set. The eMMC write is issued only from
`src/arm7/sdmmc.c::sdmmc_write()`, which itself exists only under the flag.

> **On "SCFG/MBK write-unlock".** The DSi exposes no documented SCFG or MBK bit
> that write-protects the eMMC: `MBK9` guards the NWRAM banks, and the only
> relevant gate is the ARM7 SD/MMC register-window enable (`SCFG_EXT7.bit18`,
> set by `sdmmc_enable_clock()`). The installer therefore implements the
> "unlock" as an explicit, revocable **runtime** handshake
> (`IPC_CMD_NAND_GUARD`) layered on the mandatory compile-time flag, rather than
> pretending to flip a security register that does not exist. This is stricter
> than a register unlock, not weaker: a locked ARM7 rejects every eMMC write.

### 34.7 Build gating

* Default (`make`): none of the write glue is linked. `scripts/verify_rom.py`
  asserts the linker maps contain no `sdmmc_write`/`nand_write`/`emmc_write`
  symbols, and menu `[10]` reports `GUARD: NAND WRITE LOCKED` +
  `NAND write path not built`.
* Armed (`make nand-installer`, i.e. `BUILD=build/nand
  NAND_WRITE_DANGER=1`): the full pipeline links, `[10]` runs the backup +
  verified flash and the top screen shows `GUARD: WRITE PATH ARMED`. It also
  produces `build/nand/sdcard_installer.img`, a sandbox SD image carrying
  `INSTALL.KEY` + `OLBOOT.NDS` (`make sd-image-installer` builds the same on the
  default tree). The clean `build/sdcard.img` deliberately ships **no**
  `INSTALL.KEY`, which `tests/test_installer.c` asserts.

### 34.8 Verification

`tests/test_installer.c` (suite 14) covers the SHA-256/HMAC known-answer
vectors (FIPS/RFC 4231), NAND layout validation and the protected-region
guards, the token codec (tamper/structural rejection), the pre-flight/
state-machine policy, the streaming backup + verify + flash + read-back engines,
the progress hook (`installer_percent()` + the per-sector reports), the FAT
root-file writer (multi-cluster write, duplicate rejection, read-back through
the read-only parser, `installer_sd_source_read()` zero-padding past EOF), the
locked-build report, and a simulated menu `[10]` pre-flight on the generated
`build/sdcard.img` that asserts an absent `INSTALL.KEY` is refused with
`INSTALLER_ERR_BAD_TOKEN` before any write.

`tests/test_installer_ui.c` (suite 15) covers the pure half of the modal UI
(`src/arm9/installer_ui.c`): the stage labels/indices, the bounded
`installer_ui_progress_bar()` formatter, the failure-stage selector and the
`NANDBACK.BIN` recovery line.

`make host-test` runs both after generating `build/sdcard.img`.

### 34.9 Progress reporting + modal UI

The installer engines are hardware-free, so they surface their progress through
one optional global hook (`installer_set_progress_hook()` in
`src/common/installer.c`): `installer_progress_state()` at each state
transition and `installer_progress_step()` once per streamed sector
(`installer_percent()` renders the 0..100 figure with a 64-bit intermediate).
The hook is purely observational — it has no return value and cannot alter the
transfer — so the host suites (which leave it NULL) and the on-screen renderer
share identical control flow.

Menu item `[10]` (`menu_install_view()`) drives this with
`src/arm9/installer_ui.c`:

* `installer_ui_begin()` paints the modal skeleton (both screens, `DO NOT POWER
  OFF`) and the six-stage checklist
  (`PREFLIGHT → BACKUP → VERIFY_BACKUP → UNLOCK → WRITE → VERIFY_WRITE`);
* `installer_ui_update()` — the installed hook — advances the checklist,
  redraws the progress bar/percentage and the `n/total` sector counter, and
  throttles itself to redraw only when the stage or the percentage changes;
* on `INSTALLER_STATE_DONE` the success screen reports the verified write and
  the retained `NANDBACK.BIN`; on any abort `installer_ui_show_abort()` paints
  a high-visibility failure screen that names the **exact failed stage**
  (`installer_report_t.failed_state`, recorded by `installer_fail()` on every
  abort path) and instructs the user to recover from the mandatory
  `SD:/NANDBACK.BIN` full-NAND image;
* `installer_ui_wait_back()` returns to the main menu on `B`.

In the default (locked) build `installer_run()` reports the PREFLIGHT →
ABORTED transition and the abort screen shows
`BUILD: READ-ONLY (AGENTS 5)`, so selecting `[10]` is a safe no-op.

### 34.10 Retail-NAND refusal, SD persistence, and safe real-hardware use

Three safety additions close the gaps found in the raw-staging skeleton:

* **Static shared-RAM mount volume.** `installer_run()` used to declare
  `fat_volume_t vol` on the ARM9 stack, which lives in DTCM (`0x0B000000`).
  `arm9_storage_read_sectors()` rejects any buffer outside shared Main RAM
  (`IPC_SHARED_RAM_BASE..IPC_SHARED_RAM_END`) via `ipc_addr_is_shared_ram()`, so
  the mount failed and the installer aborted at `1/6 PREFLIGHT` with
  `no SD/FAT`. The volume is now the static `s_installer_vol` (like
  `menu.c::s_fat_vol` and `storage.c::s_probe_vol`), and the invariant is
  re-checked at runtime before the first delegated read.
* **Real SD backup persistence.** See §34.5: stage `3/6` now reads
  `NANDBACK.BIN` back from the SD FAT volume and compares its size and SHA-256,
  so a dropped/`ReadOnly` SD write cannot pass unnoticed.
* **Retail/encrypted image refusal.** `nand_classify_image()` (see §34.3) feeds
  `installer_preflight()`: a retail image (absent plaintext MBR signature, or a
  `no$gba`/Unlaunch footer) is refused with
  `INSTALLER_ERR_ENCRYPTED_NAND` before the layout/token checks, and the abort
  screen names the reason.

The emulated DSi SD card must be writable for the backup to persist;
`scripts/configure_emulator.py` now writes `[DSi.SD] ReadOnly = false` by
default (and to isolated configs), with `--read-only` to opt back in.

**Safe real-hardware use (zero NAND writes).** On a console that already has
Unlaunch, chainload OpenLaunch from the SD card:

1. Build the chainload variant (`make CHAINLOAD_BOOT=1 rom`) - or plain
   `make rom` for the dry-run diagnostics build.
2. Put `build/openlaunch.nds` on the SD card as `BOOT.NDS`, or select it from
   Unlaunch's file menu. Unlaunch loads it from SD into RAM; no NAND byte is
   touched.
3. In the menu, `[10]` is safe: the default build reports
   `GUARD: NAND WRITE LOCKED` and refuses. Never build/run
   `make nand-installer` against a retail console.

Real Unlaunch-style installation (decrypted `twln`, appending to
`title/00030017/.../title.tmd`, read-only FAT attributes) is **not**
implemented; it requires the per-console AES-CTR layer (`PLAN.md` still lists
`AES / crypto` as TODO) plus `.tmd` handling. This subsystem intentionally
remains a sandbox harness until that exists.

### 34.11 Write-enabled test builds (incl. the DANGEROUS retail path)

Two additional targets exist solely for testing:

* `make installer-test` (`BUILD=build/installer-test NAND_WRITE_DANGER=1`): the
  full six-stage write pipeline, but the retail-encryption guard still applies.
  Use it with a **decrypted sandbox NAND**, or the `sdcard_installer.img`
  fixture (sandbox token), to exercise every stage and its error paths safely.
* `make installer-retail`
  (`... NAND_WRITE_DANGER=1 RETAIL_NAND_WRITE=1`): **DANGER.** The only build
  that can raw-write a retail, AES-CTR encrypted eMMC.

The retail path is the most dangerous code in the tree and is fenced four ways;
all four must hold before a byte is written:

1. **Compile flag** `ENABLE_RETAIL_NAND_WRITE` (only `make installer-retail`);
   `installer_install.c` `#error`s unless `ENABLE_NAND_WRITE_DANGER` is also set.
2. **Retail token** - the authenticated `INSTALL.KEY` must carry
   `INSTALLER_TOKEN_FLAG_RETAIL` (`make_installer_token.py --retail`).
3. **Runtime confirmation** - `menu_install_view()` shows a red DANGER screen and
   `installer_ui_confirm_retail()` blocks until the user holds `L + R + START`
   for ~2 s (B cancels).
4. **Bootsafe window** - because the retail MBR is AES-CTR encrypted,
   `nand_validate_layout()` cannot derive a protected prefix, so
   `nand_retail_region_is_safe()` hard-limits every write to sectors
   `[NAND_RETAIL_BOOT_GUARD_SECTORS, NAND_RETAIL_USABLE_SECTORS)` =
   `[0x877, 0x78000)` (byte `0x10EE00`..`0xF000000`). That keeps stage1, stage2,
   the boot-info blocks and the `0xFF800` footer - and therefore an existing
   Unlaunch install - **untouched**, so the console stays bootable.

Even with all four, a retail write is still plaintext into an encrypted
filesystem: it cannot install a working image and may corrupt the System Menu.
The mandatory, verified `NANDBACK.BIN` is the recovery path (restore with a
hardmod).

### 34.12 Roadmap: the real Unlaunch feature set

A production, safe retail installer is a multi-phase project; the raw path above
is a diagnostic placeholder. The intended phases (tracked in `PLAN.md`):

1. **AES-128 core** (`src/common/aes.c`): ECB + CTR, with host known-answer
   tests, before any decrypted read/write is possible.
2. **Key derivation**: eMMC CID (via the SD/MMC layer) + Console ID
   (`[0x4004D00]`/`[0x4004D04]`) -> the per-console NAND keyX/keyY and the CTR
   IV `SHA1(CID) + addr/16` (GBATEK "DSi SD/MMC Internal NAND Layout").
3. **Decrypted NAND access**: decrypt sector 0 to read the real MBR/partition
   table, then mount the decrypted `twln` FAT16 volume.
4. **TMD-exploit install**: locate `title/00030017/<region>/content/title.tmd`,
   append the payload to make it 81920 bytes, set the folder's FAT read-only
   attribute, keep the FAT mirrors consistent, and re-encrypt the changed
   sectors.
5. **Uninstall / coexistence**: restore the TMD to 520 bytes, clear the
   attributes, and detect an existing install (oversized TMD and/or a `0xFF800`
   footer).
6. **MBR / footer / region handling**: region variants, the nocash footer, and
   the TMD size self-check patch.

Until phases 1-4 exist, `make installer-retail` is the only way to touch retail
NAND and is explicitly unsafe for that reason.

### 34.13 Offline installer: validated crypto + SafeUnlaunch-compatible TMD edits

The per-console NAND crypto is implemented and **validated against a real retail
dump** (`src/common/aes.c`, `src/common/sha1.c`, `src/common/dsi_crypto.c`; host
suites 16 and 17). The scheme is:

```
KeyX = f(CPU/Console ID at 0x4004D00/0x4004D04)
KeyY = 0AB9DC76 BD4DC4D3 202DDD1D E1A00005
Key  = ((KeyX XOR KeyY) + FFFEFB4E295902582A680F5F1A4F3E79) ROL 42   (128-bit)
CTR  = SHA1(eMMC CID)[0:16] + (byte_address / 0x10)   (LE; byte-reversed AES)
```

`make nand-tool` builds `build/tools/dsi_nand_tool` (`info` / `decrypt` /
`encrypt` / `verify`); whole-image decrypt+re-encrypt round-trips exactly, and
the decrypted MBR/VBR match the documented DSi layout.

`scripts/dsi_nand_install.py` edits a *decrypted* image's `twln` FAT16: it
locates `title/00030017/<region>/content/title.tmd` and rewrites the payload,
updating the FAT chain (both copies) and the directory entry, then the image is
re-encrypted. It is validated against the dump by rewriting the file with
identical bytes (image unchanged) and with a different size (chain reallocated,
payload read back exactly); a full decrypt -> edit -> encrypt changes only the
intended sectors (the TMD data clusters, its directory entry and the FATs).

**SafeUnlaunch layout** (`edo9300/unlaunch-installer`, GPL-3.0 - studied, not
copied): the DSi stage2 loads a **backup launcher (`HNAA`)** if the real
launcher's (`HNAE`) TMD is unreadable. SafeUnlaunch installs Unlaunch into the
`HNAA` backup launcher's TMD and breaks one byte in the real `HNAE` TMD so stage2
falls back to it. A console already carrying this layout (all our test consoles)
is updated by **rewriting only the `HNAA` payload** - the real launcher and its
recovery path are never touched. A stock console additionally needs the `HNAE`
TMD broken.

The payload itself is boot stage-2's overflow blob (Unlaunch 2.0's is ~80 KiB,
padded to the SafeUnlaunch 196608-byte form). Producing an OpenLaunch-compatible
blob is the remaining work before a full "replace Unlaunch" install.




### Slot mux and soft-reset are controller-global (live finding)

`SD_CARD_PORT_SELECT` (`0x02`) and `SD_SOFT_RESET` (`0xE0`) belong to the
controller, not to a card instance. Two `sdmmc_t` instances (SD card and eMMC)
therefore cannot each "own" the block:

* writing `0xE0` resets **both** slots (melonDS' `DSi_SDHost::Reset()` calls
  `Ports[0]->Reset()` and `Ports[1]->Reset()`), which discards each card's
  identified state — the OCR/CSR bits in the emulator, and the selected card /
  block length on hardware;
* `0x02` selects the slot the **next** command reaches, so it has to be written
  before every command, not only at open time.

Symptom seen live (Unlaunch `bootcode.dsi` boot with `BOOT.NDS` autoboot): after
the eMMC had been opened, every "SD" read silently addressed the eMMC. LBA 63
returned zeros plus a stray `0x55`, `fat_mount()` failed, and the autoboot never
found `BOOT.NDS`. In melonDS the reset had cleared the SD card's OCR CCS bit
(bit 30), so the driver's block number was interpreted as a byte offset:
`CMD17 arg=63` became `addr=63` instead of `addr=32256`.

Fix (`src/arm7/sdmmc.c`, `src/arm7/ipc.c`):

* `sdmmc_select_port()` writes `0x02` at the start of every command and before
  the status/presence probe;
* `ipc_sd_open()` programs and soft-resets the register block **once**
  (`s_controller_ready`) and, for the second slot, only selects it and runs the
  card identification.

Regression test: `tests/test_sdmmc.c::test_port_select_before_every_command`.
