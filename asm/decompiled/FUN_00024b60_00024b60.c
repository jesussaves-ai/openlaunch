// Function: FUN_00024b60
// Address: 00024b60


void FUN_00024b60(int param_1,uint param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  
  uVar3 = param_1 << 1;
  for (; param_2 != 0; param_2 = param_2 - uVar2) {
    uVar2 = param_2;
    if (0x3000 - uVar3 < param_2) {
      uVar2 = 0x3000 - uVar3;
    }
    iVar1 = DAT_000251ec + uVar3;
    uVar3 = uVar3 + uVar2;
    if (0x2fff < uVar3) {
      uVar3 = uVar3 - 0x3000;
    }
    FUN_0001c7d0(iVar1,param_3);
    param_3 = param_3 + uVar2;
  }
  return;
}

