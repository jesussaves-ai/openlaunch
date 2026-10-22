// Function: FUN_00020e50
// Address: 00020e50


ushort * FUN_00020e50(void)

{
  ushort *puVar1;
  int iVar2;
  uint uVar3;
  ushort *unaff_r4;
  
  iVar2 = 0x20;
  uVar3 = 0;
  puVar1 = DAT_000214fc;
  do {
    if ((*puVar1 & 1) == 0) {
      return puVar1;
    }
    if (uVar3 <= *(uint *)(puVar1 + 2)) {
      unaff_r4 = puVar1;
      uVar3 = *(uint *)(puVar1 + 2);
    }
    puVar1 = puVar1 + 0x10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  FUN_00020e98(unaff_r4);
  return unaff_r4;
}

