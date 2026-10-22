// Function: FUN_000298ac
// Address: 000298ac


uint FUN_000298ac(uint param_1)

{
  uint uVar1;
  
  uVar1 = param_1 & 0xf;
  if (9 < uVar1) {
    uVar1 = uVar1 + 7;
  }
  FUN_000296b4(uVar1 + 0x30);
  return param_1;
}

