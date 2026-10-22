// Function: FUN_0602afd0
// Address:  0602afd0
// Signature: undefined FUN_0602afd0()


void FUN_0602afd0(void)

{
  int iVar1;
  
  iVar1 = DAT_0602b598;
  if ((*(ushort *)(DAT_0602b598 + 8) & 1) == 0) {
    *(ushort *)(DAT_0602b598 + 8) = *(ushort *)(DAT_0602b598 + 8) | 1;
    do {
    } while ((*(ushort *)(iVar1 + 0xc) & 4) != 0);
  }
  return;
}

