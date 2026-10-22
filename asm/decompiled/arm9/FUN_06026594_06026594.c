// Function: FUN_06026594
// Address:  06026594
// Signature: undefined FUN_06026594()


uint FUN_06026594(uint param_1)

{
  uint uVar1;
  
  uVar1 = param_1 & 0xf;
  if (9 < uVar1) {
    uVar1 = uVar1 + 7;
  }
  FUN_06026324(uVar1 + 0x30);
  return param_1;
}

