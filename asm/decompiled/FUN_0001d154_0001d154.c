// Function: FUN_0001d154
// Address: 0001d154


void FUN_0001d154(void)

{
  int iVar1;
  
  iVar1 = DAT_0001db4c;
  *(ushort *)(DAT_0001db4c + 0xe0) = *(ushort *)(DAT_0001db4c + 0xe0) & 0xfffc;
  *(ushort *)(iVar1 + 0xe0) = *(ushort *)(iVar1 + 0xe0) | 3;
  *(ushort *)(iVar1 + 8) = *(ushort *)(iVar1 + 8) & 0xfffe | 0x100;
  *(short *)(iVar1 + 0x28) = (short)DAT_0001db50;
  *(undefined2 *)(iVar1 + 0x24) = 0x40;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0x7fff;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) | 0x100;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0xfeff;
  *(undefined2 *)(iVar1 + 0x24) = 0x100;
  *(ushort *)(iVar1 + 0x38) = *(ushort *)(iVar1 + 0x38) & 0xfffe;
  *(ushort *)(iVar1 + 0x34) = *(ushort *)(iVar1 + 0x34) | 1;
  *(ushort *)(DAT_0001db54 + 4) = *(ushort *)(DAT_0001db54 + 4) & 0xfeff;
  FUN_0002a0c0(0x13,0x30);
  return;
}

