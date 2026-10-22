// Function: FUN_00029184
// Address: 00029184


void FUN_00029184(undefined4 param_1,uint param_2)

{
  uint uVar1;
  byte *unaff_r4;
  
  uVar1 = *unaff_r4 + 1;
  if (9 < (uVar1 & 0xf)) {
    uVar1 = *unaff_r4 + 7;
  }
  if (param_2 < uVar1) {
    uVar1 = 0;
  }
  *unaff_r4 = (byte)uVar1;
  return;
}

