// Function: FUN_00021414
// Address: 00021414


undefined4 FUN_00021414(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  undefined2 *puVar4;
  int iVar5;
  undefined6 uVar6;
  
  uVar6 = FUN_00020df0();
  iVar1 = (int)uVar6;
  if ((iVar1 == 0) || (iVar2 = FUN_00022378((uint)*(ushort *)(iVar1 + 2) * 2 + 0x10), iVar2 == 0)) {
    uVar3 = 0;
  }
  else {
    puVar4 = *(undefined2 **)(iVar2 + 0xc);
    puVar4[1] = (short)((uint6)uVar6 >> 0x20);
    *puVar4 = 0x100;
    puVar4[3] = 0x100;
    *(char *)(puVar4 + 2) = (char)*(undefined2 *)(iVar1 + 2);
    *(undefined1 *)((int)puVar4 + 5) = 4;
    FUN_0001c7d0(iVar1 + 0x2c,puVar4 + 4,*(undefined2 *)(iVar1 + 2));
    FUN_0001c7d0(iVar1 + 0x10,extraout_r1,4);
    FUN_0001c714(0,extraout_r1_00,*(undefined2 *)(iVar1 + 2));
    FUN_0001c92c(param_3);
    FUN_00022408(iVar2,0xe);
    iVar5 = *(int *)(iVar2 + 0xc);
    FUN_00024aa4(iVar1 + 0x2c,iVar5 + 6);
    FUN_00024aa4(DAT_0002151c,iVar5);
    *(short *)(iVar5 + 0xc) = (short)DAT_00021520;
    uVar3 = FUN_00021fa4(iVar1,iVar2);
  }
  return uVar3;
}

