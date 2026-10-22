// Function: FUN_060242c0
// Address:  060242c0
// Signature: undefined FUN_060242c0()


void FUN_060242c0(void)

{
  int unaff_r4;
  uint unaff_r5;
  int unaff_r7;
  int iVar1;
  uint uVar2;
  
  iVar1 = unaff_r4 + unaff_r7;
  uVar2 = unaff_r7 - iVar1;
  if (unaff_r5 < (uint)(unaff_r7 - iVar1)) {
    uVar2 = unaff_r5;
  }
  if (iVar1 == 0) {
    FUN_06029f04(*DAT_060250a4);
  }
  else {
    FUN_06029f04(*DAT_06025098,DAT_0602509c,iVar1 + uVar2);
    FUN_0602c1a8(DAT_060250a0 + iVar1);
  }
  return;
}

