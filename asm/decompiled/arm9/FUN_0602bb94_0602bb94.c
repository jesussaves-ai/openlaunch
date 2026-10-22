// Function: FUN_0602bb94
// Address:  0602bb94
// Signature: undefined FUN_0602bb94()


void FUN_0602bb94(void)

{
  int iVar1;
  
  iVar1 = DAT_0602bf1c;
  do {
  } while ((*(ushort *)(DAT_0602bf1c + 0x10) & 0xc) == 0xc);
  if ((*(ushort *)(DAT_0602bf1c + 0x10) & 0xc) == 0) {
    FUN_0602bc3c(1);
    *(ushort *)(iVar1 + 0x10) = *(ushort *)(iVar1 + 0x10) & 0xfff3 | 4;
    FUN_0602bc3c(1);
    *(ushort *)(iVar1 + 0x10) = *(ushort *)(iVar1 + 0x10) & 0xfff3 | 8;
    *DAT_0602bf20 = 0x20000000;
    FUN_0602bc3c(1);
  }
  return;
}

