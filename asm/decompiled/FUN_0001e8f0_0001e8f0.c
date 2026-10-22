// Function: FUN_0001e8f0
// Address: 0001e8f0


void FUN_0001e8f0(void)

{
  int iVar1;
  
  if ((uint)(*DAT_0001f6e8 - *DAT_0001f6ec) < 0x3c) {
    return;
  }
  if (*DAT_0001f6f0 != '\0') {
    return;
  }
  *DAT_0001f6ec = *DAT_0001f6e8;
  iVar1 = DAT_0001f6f4;
  *(int *)(DAT_0001f6f4 + 0xc) = *(int *)(DAT_0001f6f4 + 0xc) + 1;
  FUN_0001d548();
  *DAT_0001f880 = *(undefined1 *)(iVar1 + 1);
  return;
}

