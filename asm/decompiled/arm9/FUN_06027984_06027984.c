// Function: FUN_06027984
// Address:  06027984
// Signature: undefined FUN_06027984()


void FUN_06027984(void)

{
  undefined4 uVar1;
  undefined8 uVar2;
  
  uVar2 = FUN_06027c28();
  uVar1 = FUN_06027b04((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x54);
  *DAT_060282a8 = uVar1;
  uVar1 = FUN_06027b04(uVar1,*DAT_060282ac);
  FUN_06027b04(uVar1,*DAT_060282b0 + 0x10);
  return;
}

