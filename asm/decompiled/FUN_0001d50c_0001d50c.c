// Function: FUN_0001d50c
// Address: 0001d50c


void FUN_0001d50c(void)

{
  int iVar1;
  
  iVar1 = 0x1000;
  do {
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) {
      return;
    }
    FUN_0001d4f4();
  } while ((*DAT_0001dbf0 & 1) == 0);
  FUN_0001d930(DAT_0001dbf4,DAT_0001dbf8,1);
  return;
}

