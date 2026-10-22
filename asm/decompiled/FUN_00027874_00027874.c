// Function: FUN_00027874
// Address: 00027874


void FUN_00027874(void)

{
  int iVar1;
  
  iVar1 = DAT_000279f8;
  *(ushort *)(DAT_000279f8 + 0x38) = *(ushort *)(DAT_000279f8 + 0x38) | 2;
  *(undefined2 *)(iVar1 + 0x48) = 0;
  return;
}

