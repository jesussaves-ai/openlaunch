// Function: FUN_000263c0
// Address: 000263c0


void FUN_000263c0(uint param_1)

{
  ushort uVar1;
  
  if ((param_1 & 2) != 0) {
    if ((param_1 & 8) == 0) {
      uVar1 = *DAT_00026d34 & 0xfff7;
    }
    else {
      uVar1 = *DAT_00026d34 | 8;
    }
    *DAT_00026d34 = uVar1;
    *DAT_00026d3c = *DAT_00026d38;
    *DAT_00026d44 = *DAT_00026d40;
    *DAT_00026d4c = *DAT_00026d48;
    FUN_00025f60(DAT_00026d50,DAT_00026d54);
    FUN_00025f60(DAT_00026d58,DAT_00026d5c);
    FUN_0001c7d0(DAT_00026d60,DAT_00026d64,0x14);
    FUN_0001c7d0(DAT_00026d68,DAT_00026d6c,0x22);
    FUN_0001c7d0(DAT_00026d70,DAT_00026d74,0x10);
    *DAT_00026d7c = *DAT_00026d78;
    *DAT_00026d84 = *DAT_00026d80;
    *DAT_00026d8c = *DAT_00026d88;
    FUN_0001c7d0(DAT_00026d90,DAT_00026d94,0x20);
    *DAT_00026d9c = *DAT_00026d98;
    *DAT_00026da4 = *DAT_00026da0;
  }
  return;
}

