// Function: FUN_0602b038
// Address:  0602b038
// Signature: undefined FUN_0602b038()


void FUN_0602b038(void)

{
  int iVar1;
  
  *(ushort *)(DAT_0602b5a0 + 4) = *(ushort *)(DAT_0602b5a0 + 4) | 0x100;
  iVar1 = 0x1e;
  do {
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  return;
}

