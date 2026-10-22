// Function: FUN_00026bc8
// Address: 00026bc8


undefined2 FUN_00026bc8(int param_1,int param_2)

{
  uint uVar1;
  
  uVar1 = param_1 + param_2;
  if (7999 < uVar1) {
    uVar1 = uVar1 - 0x1340;
  }
  return *(undefined2 *)(DAT_00026e54 + uVar1);
}

