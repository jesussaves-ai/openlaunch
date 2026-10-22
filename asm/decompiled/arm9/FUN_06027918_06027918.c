// Function: FUN_06027918
// Address:  06027918
// Signature: undefined FUN_06027918()


void FUN_06027918(void)

{
  int iVar1;
  undefined8 uVar2;
  
  FUN_06027e30(DAT_06028294,*DAT_06028290 & 0xfffffffe);
  FUN_06027e30(DAT_0602829c,*DAT_06028298);
  uVar2 = FUN_06027c28(DAT_060282a0);
  FUN_06027d60((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x6c,4);
  uVar2 = FUN_06027c28(DAT_060282a4);
  FUN_06027d60((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x74,4);
  FUN_06027cd0();
  do {
    uVar2 = FUN_06027c28();
    iVar1 = FUN_06027b04((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x58);
  } while (iVar1 != 1);
  return;
}

