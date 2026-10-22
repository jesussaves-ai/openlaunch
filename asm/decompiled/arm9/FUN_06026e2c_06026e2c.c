// Function: FUN_06026e2c
// Address:  06026e2c
// Signature: undefined FUN_06026e2c()


void FUN_06026e2c(void)

{
  if (*DAT_06027498 == '\0') {
    return;
  }
  if ((*(byte *)(DAT_0602749c + 0x12) & 2) != 0) {
    return;
  }
  *DAT_060274a0 = 0;
  *DAT_060274a4 = (short)DAT_060274a8;
  FUN_0602c104(DAT_060274ac,0x80);
  FUN_0602c104(DAT_060274b0,0x80);
  FUN_0602c1a8(DAT_060274b4,DAT_060274b8,0x2000);
  FUN_0602c338();
  FUN_06027158(0x80);
  *(undefined4 *)(DAT_060274bc + 8) = 0;
  return;
}

