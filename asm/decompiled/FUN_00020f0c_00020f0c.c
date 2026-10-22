// Function: FUN_00020f0c
// Address: 00020f0c


void FUN_00020f0c(void)

{
  uint uVar1;
  ushort *puVar2;
  int iVar3;
  
  iVar3 = 0x20;
  puVar2 = DAT_00021504;
  do {
    if (((*puVar2 & 1) != 0) &&
       (*(int *)(puVar2 + 2) = *(int *)(puVar2 + 2) + 1, (*puVar2 & 2) == 0)) {
      uVar1 = puVar2[1] + 1;
      puVar2[1] = (ushort)uVar1;
      if (uVar1 < 0x7e) {
        if ((uVar1 & 7) == 7) {
          FUN_00021414(*(undefined4 *)(puVar2 + 4),*(undefined4 *)(puVar2 + 8),
                       *(undefined4 *)(puVar2 + 10));
        }
      }
      else {
        FUN_00020e98(puVar2);
      }
    }
    puVar2 = puVar2 + 0x10;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return;
}

