// Function: FUN_06027e00
// Address:  06027e00
// Signature: undefined FUN_06027e00()


void FUN_06027e00(undefined4 param_1)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_06027cb8();
  uVar2 = DAT_0602838c;
  puVar1 = DAT_06028388;
  *DAT_06028388 = 6;
  puVar1[1] = param_1;
  FUN_06028018(puVar1,uVar2,8);
  FUN_06027c58();
  return;
}

