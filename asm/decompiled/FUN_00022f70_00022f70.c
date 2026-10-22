// Function: FUN_00022f70
// Address: 00022f70


undefined4 FUN_00022f70(uint *param_1,int param_2,int *param_3)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == (uint *)0x0) {
    uVar2 = FUN_0001cd30();
    goto LAB_00022fb8;
  }
  if ((*param_1 & 0x2000) != 0) {
    if (param_2 == 1) {
      if (param_3 != (int *)0x0) {
        if (*param_3 == 0) {
          uVar3 = *param_1 & 0xffffbfff;
        }
        else {
          uVar3 = *param_1 | 0x4000;
        }
        *param_1 = uVar3;
        uVar2 = 0;
        goto LAB_00022fb8;
      }
    }
    else if ((param_2 == 2) && (param_3 != (int *)0x0)) {
      uVar3 = *param_1 & 1;
      if (uVar3 == 1) {
        uVar3 = *(uint *)(param_1[1] + 0x4c);
        uVar5 = *(uint *)(param_1[1] + 0x48);
        iVar4 = uVar3 - uVar5;
        if (uVar3 < uVar5) {
          iVar4 = iVar4 + 0x2000;
        }
        *param_3 = iVar4;
        uVar2 = 0;
        goto LAB_00022fb8;
      }
      if (uVar3 == 0) {
        iVar4 = 0;
        if (*(int **)(param_1[1] + 0x14) != (int *)0x0) {
          iVar4 = **(int **)(param_1[1] + 0x14) + -0xc;
        }
        *param_3 = iVar4;
        uVar2 = 0;
        goto LAB_00022fb8;
      }
    }
  }
  uVar2 = FUN_0001cd28();
LAB_00022fb8:
  FUN_00020af8(uVar1);
  return uVar2;
}

