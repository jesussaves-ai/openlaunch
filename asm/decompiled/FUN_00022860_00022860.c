// Function: FUN_00022860
// Address: 00022860


undefined4 FUN_00022860(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == (undefined4 *)0x0) {
    uVar2 = FUN_0001cd28();
  }
  else {
    uVar2 = 0;
    param_1[1] = 0;
    *param_1 = 0;
  }
  FUN_00020af8(uVar1);
  return uVar2;
}

