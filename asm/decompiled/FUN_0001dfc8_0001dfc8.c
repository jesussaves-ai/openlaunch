// Function: FUN_0001dfc8
// Address: 0001dfc8


int FUN_0001dfc8(int param_1)

{
  uint uVar1;
  int iVar2;
  bool bVar3;
  
  uVar1 = param_1 - 1;
  bVar3 = uVar1 == 0xd;
  if (0xd < uVar1) {
    uVar1 = 0;
  }
  iVar2 = uVar1 * 5 + DAT_0001e118;
  if (bVar3) {
    iVar2 = DAT_0001e11c;
  }
  return iVar2;
}

