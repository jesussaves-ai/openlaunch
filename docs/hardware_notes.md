# Hardware & Reverse-Engineering Notes

This document records the raw evidence extracted from `asm/UNLAUNCH.DSI`
(the reference low-level DSi bootloader). Every claim here is reproducible from
the file; nothing is invented.

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

> **Safety** — `AGENTS.md` §5 requires eMMC to be read-only by default.
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

This transport is shared with the BPTWL power-management IC. `AGENTS.md` §5
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
reviewed and gated behind `ENABLE_NAND_WRITE_DANGER` (AGENTS.md §5).
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
unimplemented, so the whole pipeline is read-only (AGENTS.md §5).

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
defined (AGENTS.md §5 sandbox-first policy). By default `loader_stage_and_boot()`
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

`pack_nds.py` gained `--title`, `--gamecode` and `--maker` options (the previous
`OPENUNLAUNCH` / `HNAA` defaults are unchanged).

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




