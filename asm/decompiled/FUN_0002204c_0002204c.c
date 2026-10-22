// Function: FUN_0002204c
// Address: 0002204c


ushort * FUN_0002204c(void)

{
  ushort *puVar1;
  int iVar2;
  
  iVar2 = 1;
  puVar1 = DAT_00022354;
  do {
    if ((*puVar1 & 1) != 0) {
      return puVar1;
    }
    puVar1 = puVar1 + 0x1a;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return (ushort *)0x0;
}

