// Function: FUN_000268f8
// Address: 000268f8


undefined4 FUN_000268f8(ushort param_1,undefined2 param_2)

{
  int iVar1;
  
  iVar1 = DAT_00026e04;
  do {
  } while ((*(ushort *)(DAT_00026e04 + 0x5e) & 1) != 0);
  *(undefined2 *)(DAT_00026e04 + 0x5a) = param_2;
  *(ushort *)(iVar1 + 0x58) = param_1 | 0x5000;
  do {
  } while ((*(ushort *)(iVar1 + 0x5e) & 1) != 0);
  return 0;
}

