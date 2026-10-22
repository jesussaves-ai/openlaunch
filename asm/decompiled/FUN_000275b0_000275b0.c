// Function: FUN_000275b0
// Address: 000275b0


void FUN_000275b0(void)

{
  int iVar1;
  undefined4 uVar2;
  
  uVar2 = FUN_00020ae4();
  iVar1 = DAT_000279bc;
  FUN_0002769c();
  FUN_00026b18(DAT_000279c0);
  FUN_00027874();
  *(ushort *)(iVar1 + 0x3c) = *(ushort *)(iVar1 + 0x3c) | 2;
  *(undefined2 *)(iVar1 + 0xac) = 0xffff;
  do {
  } while ((*(ushort *)(DAT_000279c4 + 0x9c) & 0x80) == 0);
  *DAT_000279c8 = *DAT_000279c8 | 2;
  FUN_00020af8(uVar2);
  return;
}

