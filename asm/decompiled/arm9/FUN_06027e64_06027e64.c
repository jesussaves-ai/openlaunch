// Function: FUN_06027e64
// Address:  06027e64
// Signature: undefined FUN_06027e64()


int FUN_06027e64(void)

{
  int iVar1;
  undefined8 uVar2;
  
  FUN_06027cb8();
  FUN_06027c9c(8);
  iVar1 = FUN_06027c58();
  if (iVar1 == -1) {
    uVar2 = FUN_06027c58();
    FUN_06027d50((int)uVar2,(int)((ulonglong)uVar2 >> 0x20),(int)uVar2 + -4);
    iVar1 = *DAT_06028398;
  }
  return iVar1;
}

