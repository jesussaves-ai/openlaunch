// Function: FUN_00026928
// Address: 00026928


void FUN_00026928(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = DAT_00026e08;
  do {
  } while ((*(ushort *)(DAT_00026e08 + 0x80) & 1) != 0);
  *(short *)(DAT_00026e08 + 0x7e) = (short)param_1;
  *(short *)(iVar1 + 0x7c) = (short)((uint)param_1 >> 0x10);
  do {
  } while ((*(ushort *)(iVar1 + 0x80) & 1) != 0);
  return;
}

