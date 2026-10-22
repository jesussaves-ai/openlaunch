// Function: FUN_000246dc
// Address: 000246dc


undefined4 FUN_000246dc(int param_1,undefined2 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    uVar1 = FUN_0001cd28();
  }
  else {
    FUN_00020ae4();
    if (*(int *)(param_1 + 4) != 2) {
      *(undefined2 *)(param_1 + 0x10) = param_2;
      *(undefined4 *)(param_1 + 8) = param_3;
      if (*(int *)(param_1 + 4) == 0) {
        *(undefined4 *)(param_1 + 4) = 1;
      }
    }
    FUN_00020af8();
    uVar1 = 0;
  }
  return uVar1;
}

