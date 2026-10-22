// Function: FUN_06027604
// Address:  06027604
// Signature: undefined FUN_06027604()


void FUN_06027604(void)

{
  int iVar1;
  
  FUN_06026698();
  FUN_060266d0(0x4e);
  FUN_060266d0(0x80);
  FUN_060266bc();
  FUN_06026698();
  FUN_060266d0(0x2e);
  FUN_060266d0(0);
  FUN_060266bc();
  iVar1 = DAT_06028230;
  *(ushort *)(DAT_06028230 + 0xe0) = *(ushort *)(DAT_06028230 + 0xe0) & 0xfffc;
  *(ushort *)(iVar1 + 0xe0) = *(ushort *)(iVar1 + 0xe0) | 3;
  *(ushort *)(iVar1 + 8) = *(ushort *)(iVar1 + 8) & 0xfffe | 0x100;
  *(short *)(iVar1 + 0x28) = (short)DAT_06028234;
  *(undefined2 *)(iVar1 + 0x24) = 0x40;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0x7fff;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) | 0x100;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0xfeff;
  *(undefined2 *)(iVar1 + 0x24) = 0x100;
  *(ushort *)(DAT_06028238 + 4) = *(ushort *)(DAT_06028238 + 4) & 0xfeff;
  FUN_06026f8c(0x10,0x30);
  return;
}

