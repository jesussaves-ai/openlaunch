// Function: FUN_06027dc8
// Address:  06027dc8
// Signature: undefined FUN_06027dc8()


void FUN_06027dc8(undefined4 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_06027cb8();
  uVar2 = DAT_06028384;
  puVar1 = DAT_06028380;
  *DAT_06028380 = 4;
  puVar1[1] = param_1;
  puVar1[2] = param_2;
  FUN_06028018(puVar1,uVar2,0xc);
  FUN_06027c58();
  return;
}

