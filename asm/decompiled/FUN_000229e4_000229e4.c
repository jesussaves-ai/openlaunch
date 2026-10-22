// Function: FUN_000229e4
// Address: 000229e4


undefined4 FUN_000229e4(uint *param_1,int param_2,int param_3)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 != (uint *)0x0) && (param_3 == 0x10)) && ((*param_1 & 0x2000) != 0)) {
    uVar2 = *param_1 & 1;
    if (uVar2 == 1) {
      uVar3 = FUN_0002356c(param_1[1],*(undefined2 *)(param_2 + 2),*(undefined4 *)(param_2 + 4));
      goto LAB_00022a30;
    }
    if (uVar2 == 0) {
      uVar3 = FUN_000246dc(param_1[1],*(undefined2 *)(param_2 + 2),*(undefined4 *)(param_2 + 4));
      goto LAB_00022a30;
    }
  }
  uVar3 = FUN_0001cd28();
LAB_00022a30:
  FUN_00020af8(uVar1);
  return uVar3;
}

