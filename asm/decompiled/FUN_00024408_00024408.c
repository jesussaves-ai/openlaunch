// Function: FUN_00024408
// Address: 00024408


uint FUN_00024408(int param_1)

{
  uint uVar1;
  
  uVar1 = 0;
  if (param_1 != 0) {
    uVar1 = FUN_0002249c();
    uVar1 = ~uVar1 << 0x10;
    if (uVar1 == 0) {
      uVar1 = 0xffffffff;
    }
    uVar1 = uVar1 >> 0x10;
  }
  return uVar1;
}

