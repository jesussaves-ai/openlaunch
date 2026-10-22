// Function: FUN_000298e0
// Address: 000298e0


void FUN_000298e0(uint param_1)

{
  uint uVar1;
  
  uVar1 = FUN_000298c8(param_1 >> 8 | param_1 << 0x18);
  FUN_000298c8(uVar1 >> 0x18 | uVar1 << 8);
  return;
}

