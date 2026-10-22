// Function: FUN_00023914
// Address: 00023914


uint FUN_00023914(int param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint unaff_r4;
  int iVar5;
  uint uVar6;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == 0 || param_2 == 0) {
    param_3 = FUN_0001cd28();
  }
  else {
    uVar2 = *(uint *)(param_1 + 0x4c) - *(uint *)(param_1 + 0x48);
    if (uVar2 == 0) {
      if ((*(int *)(param_1 + 0x40) == 0 && *(int *)(param_1 + 4) == 8) ||
         (*(int *)(param_1 + 0x40) == 2 && *(int *)(param_1 + 4) == 0xb)) {
        if (*(int *)(param_1 + 0x3c) == 0) {
          FUN_0001cd60();
          param_3 = 0;
        }
        else {
          *DAT_00023ac8 = *(int *)(param_1 + 0x3c);
          param_3 = 0xffffffff;
        }
      }
      else {
        param_3 = FUN_0001cd68();
      }
    }
    else {
      if (*(uint *)(param_1 + 0x4c) < *(uint *)(param_1 + 0x48)) {
        uVar2 = uVar2 + 0x2000;
      }
      if (uVar2 < param_3) {
        param_3 = uVar2;
      }
      uVar6 = *(uint *)(param_1 + 0x48);
      uVar2 = param_3;
      while (uVar2 != 0) {
        uVar4 = 0x2000 - uVar6;
        if (uVar2 < 0x2000 - uVar6) {
          uVar4 = uVar2;
        }
        iVar3 = param_1 + 0x60 + uVar6;
        uVar2 = uVar2 - uVar4;
        uVar6 = uVar6 + uVar4;
        iVar5 = param_2 + uVar4;
        FUN_0001c7d0(iVar3,param_2);
        param_2 = iVar5;
        if (0x1fff < uVar6) {
          uVar6 = uVar6 - 0x2000;
        }
      }
      if (((unaff_r4 & 0x10000000) == 0) &&
         (*(uint *)(param_1 + 0x48) = uVar6, *(int *)(param_1 + 0x44) != 0)) {
        iVar3 = *(uint *)(param_1 + 0x4c) - *(uint *)(param_1 + 0x48);
        if (*(uint *)(param_1 + 0x4c) < *(uint *)(param_1 + 0x48)) {
          iVar3 = iVar3 + 0x2000;
        }
        if (1000 < (uint)(DAT_00023ac4 - iVar3)) {
          *(undefined4 *)(param_1 + 0x44) = 0;
          FUN_0002326c(param_1,0x10,0);
        }
      }
    }
  }
  FUN_00020af8(uVar1);
  return param_3;
}

