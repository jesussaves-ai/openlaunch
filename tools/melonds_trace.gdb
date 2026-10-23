# tools/melonds_trace.gdb
#
# Dynamic trace of the DSi boot stage-2 TMD overflow in melonDS.
#
# melonDS runs the *real* DSi stage 2 from the NAND image and exposes a GDB stub,
# so we can watch the SafeUnlaunch/Unlaunch overflow on the ARM9 without any
# hardware. This traces the vulnerable launcher-TMD load (see docs/stage2_re.md).
#
# Setup (one-time):
#   ~/.config/melonDS/melonDS.toml:
#     [Instance0.Gdb]
#     Enabled = true
#     [Instance0.Gdb.ARM9]
#     BreakOnStartup = true
#     Port = 3333
#   and [DSi] NANDPath / BIOS7Path / BIOS9Path / FirmwarePath set to the dump.
#
# Run:
#   QT_QPA_PLATFORM=offscreen melonDS --boot always &
#   gdb -batch -nx -x tools/melonds_trace.gdb
#
# Addresses are the stage-2 ARM9 RAM addresses (image loads at 0x037B8000).

set architecture arm
set pagination off
set confirm off
set remotetimeout 30

target remote :3333
# BreakOnStartup leaves the ARM9 halted at the stage-2 entry (0x037B8000).

# 1. Vulnerable launcher-TMD loader.
break *0x037BAA18
continue
printf "loader: region r0=0x%08x title r1=0x%08x\n", $r0, $r1
delete 1

# 2. The read helper, only for the overflow buffer (second hit = backup launcher).
break *0x037B9C30 if $r1==0x037df06c
ignore 2 1
continue
printf "=== read into 0x037DF06C (before overflow) ===\n"
x/16x 0x037df06c

# 3. Return from the read (overflow has happened).
break *0x037b9d98
continue
delete 3
printf "=== after overflow ===\n"
x/4x 0x037df06c
printf "payload start (TMD[520]): "
x/s 0x037df06c+520

# 4. Watch for the payload entry (Unlaunch container ARM9 entry).
break *0x02200800
continue
printf "PAYLOAD ENTRY: pc=0x%08x\n", $pc
info registers r0 r1 r2 r3 sp lr
