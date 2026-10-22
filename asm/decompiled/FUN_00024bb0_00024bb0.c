// Function: FUN_00024bb0
// Address: 00024bb0


undefined2 FUN_00024bb0(int param_1,int param_2)

{
  uint uVar1;
  
  uVar1 = param_1 + param_2;
  if (0x17ff < uVar1) {
    uVar1 = uVar1 - 0x1800;
  }
  return *(undefined2 *)(DAT_000251f0 + uVar1 * 2);
}

