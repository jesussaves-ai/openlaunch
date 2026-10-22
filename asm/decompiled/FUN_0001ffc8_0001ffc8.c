// Function: FUN_0001ffc8
// Address: 0001ffc8


void FUN_0001ffc8(int param_1)

{
  int iVar1;
  undefined4 extraout_r1;
  uint uVar2;
  undefined1 uVar3;
  undefined1 auStack_38 [32];
  int local_18;
  uint uStack_14;
  
  if ((*(byte *)(param_1 + 6) & 7) == 1) {
    uVar2 = (uint)CONCAT11(*(undefined1 *)(param_1 + 0x61),*(undefined1 *)(param_1 + 0x62));
    local_18 = param_1;
    uStack_14 = uVar2;
    FUN_0001c7d0(param_1 + 0x31,auStack_38,0x10);
    FUN_0001c7d0(DAT_0002061c,extraout_r1,0x10);
    FUN_0001faa4(param_1 + 99,param_1 + 99,uVar2,auStack_38);
    return;
  }
  uVar2 = (uint)CONCAT11(*(undefined1 *)(param_1 + 0x61),*(byte *)(param_1 + 0x62));
  if (((*(byte *)(param_1 + 0x62) & 7) == 0) && (uVar3 = uVar2 == 0x10, 0xf < uVar2)) {
    iVar1 = -uVar2;
    FUN_00020a10(DAT_00020620,param_1 + 99,&stack0xfffffff0 + iVar1,uVar2);
    FUN_0001caa8(DAT_00020624,&stack0xfffffff0 + iVar1,8);
    if ((bool)uVar3) {
      FUN_0001c7d0(&stack0xfffffff8 + iVar1,param_1 + 99,uVar2 - 8);
      *(char *)(param_1 + 0x61) = (char)(uVar2 - 8 >> 8);
      *(char *)(param_1 + 0x62) = (char)(uVar2 - 8);
      return;
    }
  }
  return;
}

