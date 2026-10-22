// Function: FUN_000298c8
// Address: 000298c8


void FUN_000298c8(uint param_1)

{
  uint uVar1;
  
  uVar1 = FUN_000298ac(param_1 >> 4 | param_1 << 0x1c);
  FUN_000298ac(uVar1 >> 0x1c | uVar1 << 4);
  return;
}

