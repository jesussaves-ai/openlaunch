// Function: FUN_00020ee4
// Address: 00020ee4


void FUN_00020ee4(void)

{
  undefined2 *puVar1;
  int iVar2;
  
  iVar2 = 0x20;
  puVar1 = DAT_00021500;
  do {
    *puVar1 = 0;
    *(undefined4 *)(puVar1 + 2) = 0;
    *(undefined4 *)(puVar1 + 6) = 0;
    puVar1 = puVar1 + 0x10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}

