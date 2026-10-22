// Function: FUN_00021100
// Address: 00021100


undefined4 FUN_00021100(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  ushort *puVar3;
  undefined2 *puVar4;
  undefined4 uVar5;
  int iVar6;
  
  uVar1 = FUN_00020ae4();
  if (param_2 != 0) {
    if (param_1 == 0) {
      FUN_000223c8(param_2);
    }
    else {
      FUN_00022408(param_2,0xe);
      iVar6 = *(int *)(param_2 + 0xc);
      iVar2 = FUN_00020ec4(param_1,param_4);
      if (iVar2 != 0) {
        FUN_00024aa4(param_1 + 0x2c,iVar6 + 6);
        FUN_00024aa4(DAT_0002150c,iVar6);
LAB_000211cc:
        *(short *)(iVar6 + 0xc) = (short)param_3;
        uVar5 = FUN_00021fa4(param_1,param_2);
        goto LAB_00021238;
      }
      puVar3 = (ushort *)FUN_00020dac(param_1,param_4);
      if (puVar3 == (ushort *)0x0) {
        puVar4 = (undefined2 *)FUN_00020e50();
        *puVar4 = 1;
        *(undefined4 *)(puVar4 + 2) = 0;
        puVar4[1] = 0;
        *(int *)(puVar4 + 4) = param_1;
        *(undefined4 *)(puVar4 + 10) = param_4;
        FUN_00022408(param_2,0xfffffff2);
        *(int *)(puVar4 + 6) = param_2;
        *(undefined4 *)(puVar4 + 8) = param_3;
        FUN_00021414(param_1,param_3,param_4);
      }
      else {
        if ((*puVar3 & 2) != 0) {
          puVar3[2] = 0;
          puVar3[3] = 0;
          FUN_00024aa4(param_1 + 0x2c,iVar6 + 6);
          FUN_00024aa4(puVar3 + 0xc,iVar6);
          goto LAB_000211cc;
        }
        FUN_00022408(param_2,0xfffffff2);
        *(int *)(puVar3 + 6) = param_2;
        *(undefined4 *)(puVar3 + 8) = param_3;
      }
    }
  }
  uVar5 = 0;
LAB_00021238:
  FUN_00020af8(uVar1);
  return uVar5;
}

