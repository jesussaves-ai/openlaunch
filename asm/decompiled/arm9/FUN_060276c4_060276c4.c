// Function: FUN_060276c4
// Address:  060276c4
// Signature: undefined FUN_060276c4()


undefined4 FUN_060276c4(undefined4 param_1)

{
  int iVar1;
  uint uVar2;
  undefined4 uVar3;
  
  FUN_06027f20(param_1,4);
  iVar1 = DAT_0602823c;
  uVar3 = 0;
  if ((*(uint *)(DAT_0602823c + 0x1c) & 0x400000) != 0) {
    uVar2 = 0;
    do {
      while (FUN_06027f78(uVar2 & 0x100000), (*(uint *)(iVar1 + 0x1c) & 0x400000) != 0) {
        uVar2 = 0;
      }
      uVar2 = *(uint *)(iVar1 + 0xc);
    } while ((uVar2 & 0x80000000) == 0);
    FUN_06027f6c(0);
    FUN_06027f84(*(uint *)(iVar1 + 0xc) & 0xffff0000);
    uVar3 = 1;
  }
  return uVar3;
}

