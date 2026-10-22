// Function: FUN_060268f4
// Address:  060268f4
// Signature: undefined FUN_060268f4()


void FUN_060268f4(void)

{
  undefined2 uVar1;
  ushort extraout_r1;
  int unaff_r8;
  
  uVar1 = FUN_060268bc();
  *(undefined2 *)(unaff_r8 + 0x5a) = uVar1;
  *(ushort *)(unaff_r8 + 0x58) = extraout_r1 | 0x5000;
  FUN_060268bc();
  return;
}

