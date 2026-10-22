// Function: FUN_0001ce00
// Address: 0001ce00


void FUN_0001ce00(void)

{
  int iVar1;
  undefined2 uVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  
  iVar1 = DAT_0001cee4;
  FUN_0001c72c(DAT_0001cee4,0x80);
  *(char *)(iVar1 + 0x1e) = (char)*DAT_0001cee8;
  iVar3 = FUN_0002204c();
  uVar4 = *(undefined4 *)(iVar3 + 0x18);
  uVar5 = *(undefined4 *)(iVar3 + 0x14);
  uVar6 = *(undefined4 *)(iVar3 + 0x1c);
  *(undefined4 *)(iVar1 + 0x40) = *(undefined4 *)(iVar3 + 0x10);
  *(undefined4 *)(iVar1 + 0x44) = uVar4;
  *(undefined4 *)(iVar1 + 0x48) = uVar5;
  *(undefined4 *)(iVar1 + 0x4c) = uVar6;
  FUN_0001c7d0(DAT_0001ceec,iVar1,0x10);
  uVar2 = FUN_0001cf04(DAT_0001cef0,iVar1 + 0x12,0x6e);
  *(undefined2 *)(iVar1 + 0x10) = uVar2;
  return;
}

