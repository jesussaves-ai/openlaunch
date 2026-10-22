// Function: FUN_0602bc58
// Address:  0602bc58
// Signature: undefined FUN_0602bc58()


void FUN_0602bc58(void)

{
  int iVar1;
  
  iVar1 = DAT_0602bf2c;
  *(ushort *)(DAT_0602bf2c + 0x10) = *(ushort *)(DAT_0602bf2c + 0x10) ^ 0x8000;
  FUN_0602bc3c(1);
  *(ushort *)(iVar1 + 0x10) = *(ushort *)(iVar1 + 0x10) ^ 0x8000;
  FUN_0602bc3c(1);
  return;
}

