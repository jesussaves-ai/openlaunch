// Function: FUN_000237a8
// Address: 000237a8


uint FUN_000237a8(int param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  bool bVar7;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == 0 || param_2 == 0) {
    param_3 = FUN_0001cd28();
  }
  else if (*(int *)(param_1 + 0x40) == 0) {
    iVar2 = *(uint *)(param_1 + 0x54) - *(uint *)(param_1 + 0x50);
    bVar7 = iVar2 == 0;
    if (*(uint *)(param_1 + 0x54) < *(uint *)(param_1 + 0x50)) {
      iVar2 = iVar2 + 0x2000;
    }
    if (bVar7) {
      *(undefined4 *)(param_1 + 0x1c) = *DAT_00023ab8;
      *(undefined4 *)(param_1 + 0x20) = 500;
    }
    if ((uint)(DAT_00023abc - iVar2) < param_3) {
      param_3 = DAT_00023abc - iVar2;
    }
    uVar5 = *(uint *)(param_1 + 0x54);
    uVar6 = param_3;
    while (uVar6 != 0) {
      uVar3 = 0x2000 - uVar5;
      if (uVar6 < 0x2000 - uVar5) {
        uVar3 = uVar6;
      }
      iVar2 = DAT_00023ac0 + param_1 + uVar5;
      uVar6 = uVar6 - uVar3;
      iVar4 = param_2 + uVar3;
      uVar5 = uVar5 + uVar3;
      FUN_0001c7d0(param_2,iVar2);
      param_2 = iVar4;
      if (0x1fff < uVar5) {
        uVar5 = uVar5 - 0x2000;
      }
    }
    *(uint *)(param_1 + 0x54) = uVar5;
    uVar6 = *(uint *)(param_1 + 0x54) - *(uint *)(param_1 + 0x50);
    if (*(uint *)(param_1 + 0x54) < *(uint *)(param_1 + 0x50)) {
      uVar6 = uVar6 + 0x2000;
    }
    iVar2 = *(int *)(param_1 + 0x10) - *(int *)(param_1 + 8);
    if (((0x28 < (int)(uVar6 - iVar2)) && (*(int *)(param_1 + 4) == 5)) && (iVar2 < 1000)) {
      uVar5 = *(int *)(param_1 + 0x18) - *(int *)(param_1 + 8);
      if (uVar5 < uVar6) {
        uVar6 = uVar5;
      }
      iVar2 = FUN_0002221c(*(undefined4 *)(param_1 + 0x2c));
      if (iVar2 - 0x14U < uVar6) {
        uVar6 = iVar2 - 0x14U;
      }
      FUN_0002326c(param_1,0x10,uVar6);
      *(undefined4 *)(param_1 + 0x24) = 0;
    }
    if (param_3 == 0) {
      param_3 = FUN_0001cd68();
    }
  }
  else {
    param_3 = FUN_0001cd60();
  }
  FUN_00020af8(uVar1);
  return param_3;
}

