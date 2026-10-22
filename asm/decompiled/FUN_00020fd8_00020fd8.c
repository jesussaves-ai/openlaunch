// Function: FUN_00020fd8
// Address: 00020fd8


undefined4 FUN_00020fd8(int param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  ushort *puVar4;
  int iVar5;
  
  FUN_00020df0();
  uVar1 = FUN_00020ae4();
  if (param_2 != 0) {
    if (param_1 != 0) {
      FUN_00022408(param_2,0xfffffff2);
      iVar5 = *(int *)(param_2 + 0xc);
      iVar2 = FUN_00020edc(param_1,param_2,iVar5);
      if (iVar2 != 0) {
        FUN_00022408(param_2,0xe);
        if (*(short *)(iVar5 + 6) == 0x100) {
          iVar2 = FUN_0001c8d8(0x100,iVar5 + (uint)*(byte *)(iVar5 + 4) * 2 + 0xc);
          if (*(int *)(param_1 + 0x10) == iVar2) {
            FUN_0002124c(param_1,param_2);
            goto LAB_00021050;
          }
        }
        else if (*(short *)(iVar5 + 6) == 0x200) {
          uVar3 = FUN_0001c8d8(0x200,iVar5 + 8 + (uint)*(byte *)(iVar5 + 4));
          puVar4 = (ushort *)FUN_00020dac(param_1,uVar3);
          if (puVar4 != (ushort *)0x0) {
            FUN_0001c7d0(iVar5 + 8,puVar4 + 0xc,*(undefined1 *)(iVar5 + 4));
            *puVar4 = *puVar4 | 2;
            iVar2 = *(int *)(puVar4 + 6);
            if (iVar2 != 0) {
              puVar4[6] = 0;
              puVar4[7] = 0;
              FUN_00021100(param_1,iVar2,*(undefined4 *)(puVar4 + 8),uVar3);
            }
          }
        }
      }
    }
    FUN_000223c8(param_2);
  }
LAB_00021050:
  FUN_00020af8(uVar1);
  return 0;
}

