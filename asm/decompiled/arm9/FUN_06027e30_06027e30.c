// Function: FUN_06027e30
// Address:  06027e30
// Signature: undefined FUN_06027e30()


void FUN_06027e30(undefined4 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_06027cb8();
  uVar2 = DAT_06028394;
  puVar1 = DAT_06028390;
  *DAT_06028390 = 7;
  puVar1[1] = param_1;
  puVar1[2] = param_2;
  FUN_06028018(puVar1,uVar2,0xc);
  return;
}

