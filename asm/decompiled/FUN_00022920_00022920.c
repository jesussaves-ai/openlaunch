// Function: FUN_00022920
// Address: 00022920


undefined4 FUN_00022920(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == (uint *)0x0) {
LAB_00022958:
    uVar3 = FUN_0001cd28();
  }
  else {
    if ((*param_1 & 0x2000) == 0) {
      uVar3 = 0;
      goto LAB_00022974;
    }
    uVar2 = *param_1 & 1;
    if (uVar2 == 1) {
      iVar4 = *(int *)(param_1[1] + 4);
      if (((iVar4 != 0xc && iVar4 != 1) && iVar4 != 0) && iVar4 != 2) {
        FUN_00022f18(param_1,0);
        *param_1 = *param_1 & ~DAT_00023068 | DAT_0002306c;
        uVar3 = 0;
        goto LAB_00022974;
      }
      FUN_000234a4();
    }
    else {
      if (uVar2 != 0) goto LAB_00022958;
      FUN_00024680(param_1[1]);
    }
    uVar3 = 0;
    param_1[1] = 0;
    *param_1 = 0;
  }
LAB_00022974:
  FUN_00020af8(uVar1);
  return uVar3;
}

