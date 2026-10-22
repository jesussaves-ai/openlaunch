// Function: FUN_000222fc
// Address: 000222fc


int FUN_000222fc(int param_1,uint param_2)

{
  ushort *puVar1;
  int iVar2;
  
  if (param_1 != 0) {
    return param_1;
  }
  iVar2 = 1;
  puVar1 = DAT_0002234c;
  while (((*puVar1 & 1) != 0 || ((*(uint *)(puVar1 + 0xc) & (*(uint *)(puVar1 + 8) ^ param_2)) != 0)
         )) {
    puVar1 = puVar1 + 0x1a;
    iVar2 = iVar2 + -1;
    if (iVar2 == 0) {
      iVar2 = 1;
      puVar1 = DAT_00022350;
      do {
        if ((*puVar1 & 1) != 0) {
          return *(int *)(puVar1 + 8);
        }
        puVar1 = puVar1 + 0x1a;
        iVar2 = iVar2 + -1;
      } while (iVar2 != 0);
      return 0;
    }
  }
  return *(int *)(puVar1 + 8);
}

