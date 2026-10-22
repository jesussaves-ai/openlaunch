// Function: FUN_00021eac
// Address: 00021eac


undefined4 FUN_00021eac(ushort *param_1,int param_2)

{
  undefined4 uVar1;
  ushort *puVar2;
  int iVar3;
  uint uVar4;
  
  if (param_2 != 0) {
    if ((param_1 != (ushort *)0x0) && ((*param_1 & 0x8000) != 0)) {
      uVar4 = (uint)*(ushort *)(*(int *)(param_2 + 0xc) + 0xc);
      if (uVar4 == DAT_0002233c) {
        FUN_00020fd8();
        return 0;
      }
      FUN_00022408(param_2,0xfffffff2);
      iVar3 = 1;
      puVar2 = DAT_00022340;
      do {
        if (((*puVar2 & 0x8000) != 0) && (puVar2[1] == uVar4)) {
          uVar1 = FUN_0001c968(param_2,puVar2,iVar3,*(undefined4 *)(puVar2 + 2));
          return uVar1;
        }
        puVar2 = puVar2 + 4;
        iVar3 = iVar3 + -1;
      } while (iVar3 != 0);
    }
    FUN_000223c8(param_2);
  }
  return 0;
}

