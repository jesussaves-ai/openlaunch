// Function: FUN_06027aac
// Address:  06027aac
// Signature: undefined FUN_06027aac()


void FUN_06027aac(int param_1)

{
  ushort uVar1;
  int iVar2;
  int extraout_r1;
  int iVar3;
  uint uVar4;
  
  uVar1 = *(ushort *)(param_1 + 2);
  iVar3 = uVar1 + 6;
  if (param_1 == DAT_060282e8) {
    iVar2 = DAT_060282e8 + iVar3;
  }
  else {
    FUN_0602c1a8(param_1,DAT_060282e8,iVar3);
    iVar2 = extraout_r1;
  }
  FUN_0602c104(iVar2,-iVar3 & 0x7f);
  uVar4 = uVar1 + 0x85;
  FUN_06028018(DAT_060282ec,DAT_060282f0 - (uVar4 & 0xffffff80),uVar4 >> 7);
  return;
}

