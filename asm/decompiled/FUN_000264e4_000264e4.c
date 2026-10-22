// Function: FUN_000264e4
// Address: 000264e4


int FUN_000264e4(int param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  
  iVar2 = 0;
  if (*DAT_00026db0 != *DAT_00026db4) {
    iVar2 = FUN_000264c0(5);
    uVar5 = *DAT_00026db8;
    uVar3 = *DAT_00026dbc - uVar5;
    if (*DAT_00026dbc < uVar5) {
      uVar3 = uVar3 + 0x3000;
    }
    uVar6 = iVar2 + 9U >> 1;
    if (uVar3 < uVar6) {
      iVar2 = 0;
    }
    else {
      while (uVar6 != 0) {
        uVar3 = uVar6;
        if (0x3000 - uVar5 < uVar6) {
          uVar3 = 0x3000 - uVar5;
        }
        iVar1 = uVar5 * 2;
        uVar5 = uVar5 + uVar3;
        iVar4 = param_1 + uVar3 * 2;
        uVar6 = uVar6 - uVar3;
        FUN_0001c7d0(DAT_00026dc0 + iVar1,param_1,uVar3 << 1);
        param_1 = iVar4;
        if (0x2fff < uVar5) {
          uVar5 = uVar5 - 0x3000;
        }
      }
      *DAT_00026dc4 = uVar5;
      *DAT_00026dc8 = 0;
      iVar1 = DAT_00026dcc;
      *(int *)(DAT_00026dcc + 9) = *(int *)(DAT_00026dcc + 9) + 1;
      *(int *)(iVar1 + 10) = *(int *)(iVar1 + 10) + iVar2 + 8;
      *(int *)(iVar1 + 0xb) = *(int *)(iVar1 + 0xb) + iVar2 + -4;
    }
  }
  return iVar2;
}

