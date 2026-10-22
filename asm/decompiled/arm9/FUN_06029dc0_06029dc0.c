// Function: FUN_06029dc0
// Address:  06029dc0
// Signature: undefined FUN_06029dc0()


void FUN_06029dc0(void)

{
  int iVar1;
  
  FUN_060288d4(0x41);
  FUN_06029f00(1,DAT_0602a370);
  iVar1 = DAT_0602a374;
  *(undefined4 *)(DAT_0602a374 + 0x24) = *(undefined4 *)(DAT_0602a374 + 0x28);
  *(undefined4 *)(iVar1 + 0x34) = *(undefined4 *)(iVar1 + 0x38);
  *(undefined1 *)(iVar1 + 0x12) = 3;
  *(undefined4 *)(iVar1 + 0x1b8) = 0xffffffff;
  FUN_0602c1a8(DAT_0602a378,DAT_0602a37c,0x160);
  return;
}

