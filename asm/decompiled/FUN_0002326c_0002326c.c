// Function: FUN_0002326c
// Address: 0002326c


undefined4 FUN_0002326c(int param_1,undefined4 param_2,uint param_3)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  
  uVar1 = FUN_00020ae4();
  if (param_1 != 0) {
    uVar2 = *(uint *)(param_1 + 0x54) - *(uint *)(param_1 + 0x50);
    if (*(uint *)(param_1 + 0x54) < *(uint *)(param_1 + 0x50)) {
      uVar2 = uVar2 + 0x2000;
    }
    if (uVar2 < param_3) {
      param_3 = uVar2;
    }
    iVar3 = FUN_0002319c(param_1,param_2,param_3);
    if (iVar3 != 0) {
      *(uint *)(param_1 + 0x10) = *(int *)(param_1 + 8) + param_3;
      uVar2 = *(uint *)(param_1 + 0x50);
      while (param_3 != 0) {
        uVar4 = 0x2000 - uVar2;
        if (param_3 < 0x2000 - uVar2) {
          uVar4 = param_3;
        }
        FUN_00022650(iVar3,uVar2 + DAT_00023a94 + param_1);
        param_3 = param_3 - uVar4;
        uVar2 = uVar2 + uVar4;
        if (0x1fff < uVar2) {
          uVar2 = uVar2 - 0x2000;
        }
      }
      FUN_00023164(iVar3,*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x2c));
      FUN_0002223c(iVar3,6,*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x2c));
      *(undefined4 *)(param_1 + 0x1c) = *DAT_00023a98;
      *(undefined4 *)(param_1 + 0x20) = 500;
    }
  }
  FUN_00020af8(uVar1);
  return 0;
}

