// Function: FUN_00029188
// Address: 00029188


void FUN_00029188(uint param_1,uint param_2)

{
  uint uVar1;
  byte *unaff_r4;
  
  uVar1 = *unaff_r4 + 1;
  if (9 < (uVar1 & 0xf)) {
    uVar1 = *unaff_r4 + 7;
  }
  if (param_2 < uVar1) {
    uVar1 = param_1;
  }
  *unaff_r4 = (byte)uVar1;
  return;
}

