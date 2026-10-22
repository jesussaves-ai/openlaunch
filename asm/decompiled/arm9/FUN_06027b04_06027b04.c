// Function: FUN_06027b04
// Address:  06027b04
// Signature: undefined FUN_06027b04()


void FUN_06027b04(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  uint *puVar1;
  undefined4 uVar2;
  
  uVar2 = DAT_060282f8;
  puVar1 = DAT_060282f4;
  *DAT_060282f4 = param_2 >> 8;
  FUN_06028018(puVar1,uVar2,3,param_2 >> 8,param_2,param_3,param_4);
  uVar2 = DAT_06028300;
  puVar1 = DAT_060282fc;
  *DAT_060282fc = param_2 & 0xff;
  uVar2 = FUN_06028018(puVar1,uVar2,1);
  FUN_06027c5c(uVar2,DAT_06028304);
  return;
}

