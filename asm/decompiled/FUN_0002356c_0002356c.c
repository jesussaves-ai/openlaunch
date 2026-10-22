// Function: FUN_0002356c
// Address: 0002356c


undefined4 FUN_0002356c(int param_1,undefined2 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00020ae4();
  if ((param_1 != 0) && (*(int *)(param_1 + 4) == 0)) {
    *(undefined4 *)(param_1 + 0x28) = param_3;
    *(undefined2 *)(param_1 + 0x30) = param_2;
    *(undefined4 *)(param_1 + 4) = 1;
  }
  FUN_00020af8(uVar1);
  return 0;
}

