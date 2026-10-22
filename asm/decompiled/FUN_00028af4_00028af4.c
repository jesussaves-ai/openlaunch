// Function: FUN_00028af4
// Address: 00028af4


void FUN_00028af4(void)

{
  int iVar1;
  
  iVar1 = 8;
  FUN_0001caa8(DAT_00028d00,DAT_00028d04);
  if (iVar1 != 0) {
    FUN_0001c72c(DAT_00028d08,0x90);
  }
  *DAT_00028d0c = 1;
  do {
  } while (*DAT_00028d10 != '\0');
  if (*DAT_00028d14 != 0 || DAT_00028d14[1] != 0) {
    FUN_00028b78(DAT_00028d14,DAT_00028d18,8);
    FUN_00020be4(6,0);
  }
  *DAT_00028d1c = 0;
  return;
}

