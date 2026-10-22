// Function: FUN_060265c8
// Address:  060265c8
// Signature: undefined FUN_060265c8()


void FUN_060265c8(uint param_1)

{
  uint uVar1;
  
  uVar1 = FUN_060265b0(param_1 >> 8 | param_1 << 0x18);
  FUN_060265b0(uVar1 >> 0x18 | uVar1 << 8);
  return;
}

