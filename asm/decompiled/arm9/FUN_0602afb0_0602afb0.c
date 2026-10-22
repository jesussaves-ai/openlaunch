// Function: FUN_0602afb0
// Address:  0602afb0
// Signature: undefined FUN_0602afb0()


void FUN_0602afb0(void)

{
  int iVar1;
  
  iVar1 = DAT_0602b594;
  *(ushort *)(DAT_0602b594 + 4) = *(ushort *)(DAT_0602b594 + 4) & 0xfffe;
  *(ushort *)(iVar1 + 4) = *(ushort *)(iVar1 + 4) & 0xfffd;
  return;
}

