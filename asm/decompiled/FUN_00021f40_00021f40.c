// Function: FUN_00021f40
// Address: 00021f40


undefined4 FUN_00021f40(undefined4 param_1,int param_2,uint param_3,uint param_4)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  if (param_2 != 0) {
    iVar3 = 1;
    iVar2 = DAT_00022344;
    do {
      if (*(uint *)(iVar2 + 0x10) == param_4) {
        if (((param_4 ^ param_3) & *(uint *)(iVar2 + 0x18)) != 0 && param_3 != 0xffffffff) {
          param_3 = *(uint *)(iVar2 + 0x14);
        }
        uVar1 = FUN_00021100(iVar2,param_2,param_1,param_3);
        return uVar1;
      }
      iVar2 = iVar2 + 0x34;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    FUN_000223c8(param_2);
  }
  return 0;
}

