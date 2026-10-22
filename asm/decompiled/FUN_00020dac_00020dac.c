// Function: FUN_00020dac
// Address: 00020dac


ushort * FUN_00020dac(uint param_1,int param_2)

{
  ushort *puVar1;
  int iVar2;
  uint uVar3;
  bool bVar4;
  bool bVar5;
  
  iVar2 = 0x20;
  puVar1 = DAT_000214f8;
  while( true ) {
    uVar3 = *puVar1 ^ 1;
    bVar4 = (uVar3 & 1) == 0;
    if (bVar4) {
      uVar3 = *(uint *)(puVar1 + 4);
    }
    bVar5 = bVar4 && uVar3 == param_1;
    if (bVar4 && uVar3 == param_1) {
      bVar5 = *(int *)(puVar1 + 10) == param_2;
    }
    if (bVar5) break;
    puVar1 = puVar1 + 0x10;
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) {
      return (ushort *)0x0;
    }
  }
  return puVar1;
}

