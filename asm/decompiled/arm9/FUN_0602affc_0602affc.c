// Function: FUN_0602affc
// Address:  0602affc
// Signature: undefined FUN_0602affc()


ushort FUN_0602affc(void)

{
  ushort uVar1;
  int iVar2;
  
  iVar2 = DAT_0602b59c;
  uVar1 = *(ushort *)(DAT_0602b59c + 8);
  if ((uVar1 & 1) != 0) {
    *(ushort *)(DAT_0602b59c + 8) = *(ushort *)(DAT_0602b59c + 8) & 0xf1ff;
    *(undefined2 *)(iVar2 + 0x10) = 0;
    *(undefined2 *)(iVar2 + 0x18) = 0xffff;
    uVar1 = *(ushort *)(iVar2 + 0x34);
  }
  return uVar1;
}

