// Function: FUN_0001d548
// Address: 0001d548


void FUN_0001d548(int param_1)

{
  ushort uVar1;
  int iVar2;
  int extraout_r1;
  int iVar3;
  uint uVar4;
  
  uVar1 = *(ushort *)(param_1 + 2);
  iVar3 = uVar1 + 6;
  if (param_1 == DAT_0001dbfc) {
    iVar2 = DAT_0001dbfc + iVar3;
  }
  else {
    FUN_0001c7d0(param_1,DAT_0001dbfc,iVar3);
    iVar2 = extraout_r1;
  }
  FUN_0001c72c(iVar2,-iVar3 & 0x7f);
  uVar4 = uVar1 + 0x85;
  FUN_0001d958(DAT_0001dc00,DAT_0001dc04 - (uVar4 & 0xffffff80),uVar4 >> 7);
  return;
}

