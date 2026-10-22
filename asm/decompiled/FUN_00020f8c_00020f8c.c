// Function: FUN_00020f8c
// Address: 00020f8c


void FUN_00020f8c(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_00020ae4();
  iVar3 = 0x20;
  iVar2 = DAT_00021508;
  do {
    if (param_1 == *(int *)(iVar2 + 8) || param_1 == 0) {
      FUN_00020e98(iVar2);
    }
    iVar2 = iVar2 + 0x20;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  FUN_00020af8(uVar1);
  return;
}

