// Function: FUN_0002124c
// Address: 0002124c


undefined4 FUN_0002124c(int param_1,int param_2)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 extraout_r1;
  int iVar3;
  
  if (param_2 != 0) {
    if (param_1 != 0) {
      FUN_00022408(param_2,0xfffffff2);
      iVar3 = *(int *)(param_2 + 0xc);
      iVar1 = FUN_00020edc(iVar3);
      if ((iVar1 != 0) &&
         ((ushort)*(byte *)(iVar3 + 4) == *(ushort *)(param_1 + 2) && *(char *)(iVar3 + 5) == '\x04'
         )) {
        *(undefined2 *)(iVar3 + 6) = 0x200;
        FUN_0001c7d0(iVar3 + 8,iVar3 + 8 + *(ushort *)(param_1 + 2) + 4);
        FUN_0001c7d0(param_1 + 0x2c,iVar3 + 8,*(undefined2 *)(param_1 + 2));
        FUN_0001c7d0(param_1 + 0x10,extraout_r1,4);
        FUN_00022408(param_2,0xe);
        iVar1 = *(int *)(param_2 + 0xc);
        FUN_00024aa4(param_1 + 0x2c,iVar1 + 6);
        FUN_00024aa4(iVar3 + (uint)*(ushort *)(param_1 + 2) + 0xc,iVar1);
        *(short *)(iVar1 + 0xc) = (short)DAT_00021510;
        uVar2 = FUN_00021fa4(param_1,param_2);
        return uVar2;
      }
    }
    FUN_000223c8(param_2);
  }
  return 0;
}

