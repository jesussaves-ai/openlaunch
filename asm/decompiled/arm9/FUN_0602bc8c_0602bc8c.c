// Function: FUN_0602bc8c
// Address:  0602bc8c
// Signature: undefined FUN_0602bc8c()


void FUN_0602bc8c(void)

{
  uint uVar1;
  int unaff_r4;
  
  uVar1 = *(uint *)(DAT_0602bf30 + 0x60) | 0x20000000;
  if ((*DAT_0602bf34 & 0x80000000) == 0) {
    uVar1 = uVar1 & ~DAT_0602bf38 | 0x180000;
  }
  else if ((0x17f < (DAT_0602bf3c & uVar1)) && ((*(uint *)(DAT_0602bf30 + 0x60) & 0x10000000) == 0))
  {
    uVar1 = uVar1 ^ DAT_0602bf3c & uVar1 | 0x180;
  }
  *(uint *)(unaff_r4 + 0x1a4) = uVar1;
  return;
}

