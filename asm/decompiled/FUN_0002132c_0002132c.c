// Function: FUN_0002132c
// Address: 0002132c


undefined4 FUN_0002132c(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  undefined4 extraout_r1_01;
  undefined2 *puVar4;
  int iVar5;
  
  uVar1 = FUN_00020ae4();
  if ((param_1 == 0) ||
     (iVar2 = FUN_00022378((uint)*(ushort *)(param_1 + 2) * 2 + 0x10), iVar2 == 0)) {
    uVar3 = 0;
  }
  else {
    puVar4 = *(undefined2 **)(iVar2 + 0xc);
    puVar4[1] = 8;
    *puVar4 = 0x100;
    puVar4[3] = 0x100;
    *(char *)(puVar4 + 2) = (char)*(undefined2 *)(param_1 + 2);
    *(undefined1 *)((int)puVar4 + 5) = 4;
    FUN_0001c7d0(param_1 + 0x2c,puVar4 + 4,*(undefined2 *)(param_1 + 2));
    FUN_0001c7d0(param_1 + 0x10,extraout_r1,4);
    FUN_0001c714(0,extraout_r1_00,*(undefined2 *)(param_1 + 2));
    FUN_0001c7d0(param_1 + 0x10,extraout_r1_01,4);
    FUN_00022408(iVar2,0xe);
    iVar5 = *(int *)(iVar2 + 0xc);
    FUN_00024aa4(param_1 + 0x2c,iVar5 + 6);
    FUN_00024aa4(DAT_00021514,iVar5);
    *(short *)(iVar5 + 0xc) = (short)DAT_00021518;
    uVar3 = FUN_00021fa4(param_1,iVar2);
  }
  FUN_00020af8(uVar1);
  return uVar3;
}

