// Function: FUN_000268cc
// Address: 000268cc


undefined2 FUN_000268cc(ushort param_1)

{
  int iVar1;
  
  iVar1 = DAT_00026e00;
  do {
  } while ((*(ushort *)(DAT_00026e00 + 0x5e) & 1) != 0);
  *(ushort *)(DAT_00026e00 + 0x58) = param_1 | 0x6000;
  do {
  } while ((*(ushort *)(iVar1 + 0x5e) & 1) != 0);
  return *(undefined2 *)(iVar1 + 0x5c);
}

