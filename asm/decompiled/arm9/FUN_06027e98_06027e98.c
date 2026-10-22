// Function: FUN_06027e98
// Address:  06027e98
// Signature: undefined FUN_06027e98()


void FUN_06027e98(undefined4 param_1)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_06027cb8();
  uVar2 = DAT_060283a0;
  puVar1 = DAT_0602839c;
  *DAT_0602839c = 0xd;
  puVar1[1] = param_1;
  FUN_06028018(puVar1,uVar2,8);
  return;
}

