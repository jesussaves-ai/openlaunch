// Function: FUN_000223c8
// Address: 000223c8


void FUN_000223c8(undefined4 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00020ae4();
  while (param_1 != (undefined4 *)0x0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1 = (undefined4 *)param_1[2];
    FUN_000249f8();
  }
  FUN_00020af8(uVar1);
  return;
}

