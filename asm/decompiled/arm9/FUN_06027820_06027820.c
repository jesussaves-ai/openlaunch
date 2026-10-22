// Function: FUN_06027820
// Address:  06027820
// Signature: undefined FUN_06027820()


void FUN_06027820(void)

{
  undefined4 uVar1;
  uint uVar2;
  undefined8 uVar3;
  
  uVar1 = FUN_06027e64();
  *DAT_06028260 = uVar1;
  uVar3 = FUN_06027c28(DAT_06028264);
  FUN_06027d60((int)uVar3,(int)((ulonglong)uVar3 >> 0x20),4);
  uVar2 = FUN_06027e00(DAT_06028268);
  *DAT_0602826c = uVar2;
  FUN_06027e30(DAT_06028270,uVar2 | 8);
  uVar2 = FUN_06027e00(DAT_06028274);
  *DAT_06028278 = uVar2;
  FUN_06027e30(DAT_0602827c,uVar2 | 1);
  FUN_06027e30(DAT_06028280,5);
  FUN_06027e30(DAT_06028284,0);
  return;
}

