// Function: FUN_000265c8
// Address: 000265c8


undefined4 FUN_000265c8(uint param_1,uint param_2,uint param_3,int param_4)

{
  int *piVar1;
  uint *puVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  
  piVar1 = DAT_00026ddc;
  iVar4 = *DAT_00026dd0 - *DAT_00026dd4;
  if (*DAT_00026dd0 < *DAT_00026dd4 || iVar4 == 0) {
    iVar4 = iVar4 + 0x1800;
  }
  if ((uint)((iVar4 + -1) * 2) < param_2) {
    *(int *)(DAT_00026dd8 + 0x10) = *(int *)(DAT_00026dd8 + 0x10) + 1;
    FUN_000297c4();
    uVar3 = 0;
  }
  else {
    *DAT_00026ddc = *DAT_00026ddc + 1;
    piVar1[2] = piVar1[2] + param_2;
    puVar2 = DAT_00026de0;
    uVar8 = *DAT_00026de0;
    while( true ) {
      uVar7 = param_1;
      if (param_3 <= param_1) {
        uVar7 = param_1 - param_4;
      }
      if (0x17ff < uVar8) {
        uVar8 = uVar8 - 0x1800;
      }
      if (param_2 == 0) break;
      uVar6 = uVar8 * -2 + 0x3000;
      uVar5 = param_2;
      if (uVar6 < param_2) {
        uVar5 = uVar6;
      }
      if (param_3 - uVar7 < uVar5) {
        uVar5 = param_3 - uVar7;
      }
      iVar4 = uVar8 * 2;
      param_1 = uVar7 + uVar5;
      uVar8 = uVar8 + (uVar5 >> 1);
      param_2 = param_2 - uVar5;
      FUN_0001c7d0(uVar7,DAT_00026de4 + iVar4);
    }
    *puVar2 = uVar8;
    FUN_0001cc34();
    uVar3 = 1;
  }
  return uVar3;
}

