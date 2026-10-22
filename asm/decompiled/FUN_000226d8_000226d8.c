// Function: FUN_000226d8
// Address: 000226d8


void FUN_000226d8(void)

{
  undefined4 *puVar1;
  int iVar2;
  
  iVar2 = 0x20;
  puVar1 = DAT_00023054;
  do {
    puVar1[1] = 0;
    *puVar1 = 0;
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}

