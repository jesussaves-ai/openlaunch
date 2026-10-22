// Function: FUN_0602bc00
// Address:  0602bc00
// Signature: undefined FUN_0602bc00()


void FUN_0602bc00(void)

{
  int iVar1;
  ushort uVar2;
  
  iVar1 = DAT_0602bf24;
  do {
    uVar2 = *(ushort *)(DAT_0602bf24 + 0x10) & 0xc;
  } while (uVar2 == 0xc);
  if (uVar2 == 8) {
    *(ushort *)(DAT_0602bf24 + 0x10) = *(ushort *)(DAT_0602bf24 + 0x10) | 0xc;
    do {
    } while ((*(ushort *)(iVar1 + 0x10) & 0xc) != 0);
  }
  return;
}

