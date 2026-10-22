// Function: FUN_0001fcd4
// Address: 0001fcd4


void FUN_0001fcd4(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  
  uVar1 = *(uint *)(param_1 + 0x14);
  *(uint *)(param_1 + 0x14) = uVar1 + param_3 * 8;
  *(uint *)(param_1 + 0x18) =
       *(int *)(param_1 + 0x18) + (param_3 >> 0x1d) + (uint)CARRY4(uVar1,param_3 * 8);
  iVar4 = *(int *)(param_1 + 0x5c);
  uVar1 = param_2;
  if ((iVar4 != 0) && (0x3f < iVar4 + param_3)) {
    uVar1 = param_2 + (0x40 - iVar4);
    param_3 = param_3 - (0x40 - iVar4);
    FUN_0001c7d0(param_2,param_1 + 0x1c + iVar4);
    FUN_0001fdbc(param_1,param_1 + 0x1c,0x40);
    iVar4 = 0;
  }
  if (0x3f < param_3) {
    uVar3 = uVar1;
    if ((uVar1 & 3) != 0) {
      do {
        uVar3 = uVar1 + 0x40;
        param_3 = param_3 - 0x40;
        FUN_0001c7d0(uVar1,param_1 + 0x1c);
        FUN_0001fdbc(param_1,param_1 + 0x1c,0x40);
        uVar1 = uVar3;
      } while (0x3f < param_3);
    }
    uVar2 = param_3 & 0xffffffc0;
    uVar1 = uVar3 + uVar2;
    param_3 = param_3 - uVar2;
    if (uVar2 != 0) {
      FUN_0001fdbc(param_1,uVar3);
    }
  }
  *(uint *)(param_1 + 0x5c) = iVar4 + param_3;
  FUN_0001c7d0(uVar1,param_1 + 0x1c + iVar4);
  return;
}

