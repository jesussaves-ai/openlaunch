// Function: FUN_00024b10
// Address: 00024b10


void FUN_00024b10(int param_1,int param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar3 = param_1 << 1;
  for (uVar4 = param_2 << 1; uVar4 != 0; uVar4 = uVar4 - uVar2) {
    uVar2 = uVar4;
    if (0x6000 - uVar3 < uVar4) {
      uVar2 = 0x6000 - uVar3;
    }
    iVar1 = DAT_000251e8 + uVar3;
    uVar3 = uVar3 + uVar2;
    if (0x5fff < uVar3) {
      uVar3 = uVar3 - 0x6000;
    }
    FUN_0001c7d0(param_3,iVar1);
    param_3 = param_3 + uVar2;
  }
  return;
}

