// Function: FUN_000200d8
// Address: 000200d8


void FUN_000200d8(int param_1)

{
  short sVar1;
  short sVar2;
  
  sVar1 = CONCAT11(*(undefined1 *)(param_1 + 0x61),*(undefined1 *)(param_1 + 0x62));
  if (*DAT_00020628 == '\x03') {
    sVar2 = 0x20;
  }
  else {
    sVar2 = 0x10;
  }
  if (sVar2 != sVar1) {
    return;
  }
  FUN_0001c7d0(param_1 + 99,DAT_0002062c,sVar1);
  *DAT_00020630 = (char)((*(byte *)(param_1 + 6) & 0x30) >> 4);
  return;
}

