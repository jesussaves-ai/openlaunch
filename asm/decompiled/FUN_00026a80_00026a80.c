// Function: FUN_00026a80
// Address: 00026a80


void FUN_00026a80(void)

{
  undefined1 *puVar1;
  int iVar2;
  
  *(undefined2 *)(DAT_00026e28 + 0x60) = 0x100;
  iVar2 = 0;
  puVar1 = DAT_00026e2c;
  do {
    FUN_000268f8(iVar2,*puVar1);
    iVar2 = iVar2 + 1;
    puVar1 = puVar1 + 1;
  } while (iVar2 != 0x69);
  return;
}

