// Function: FUN_060268d8
// Address:  060268d8
// Signature: undefined FUN_060268d8()


void FUN_060268d8(void)

{
  undefined4 uVar1;
  int unaff_r8;
  
  uVar1 = FUN_060268a0();
  *(short *)(unaff_r8 + 0x7e) = (short)uVar1;
  *(short *)(unaff_r8 + 0x7c) = (short)((uint)uVar1 >> 0x10);
  FUN_060268a0();
  return;
}

