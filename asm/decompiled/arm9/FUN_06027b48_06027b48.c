// Function: FUN_06027b48
// Address:  06027b48
// Signature: undefined FUN_06027b48()


undefined8 FUN_06027b48(undefined4 param_1,uint param_2)

{
  uint *puVar1;
  undefined4 uVar2;
  
  FUN_06027ca0(param_1,DAT_06028308);
  uVar2 = DAT_06028310;
  puVar1 = DAT_0602830c;
  *DAT_0602830c = param_2 >> 8;
  FUN_06028018(puVar1,uVar2,3);
  uVar2 = DAT_06028318;
  puVar1 = DAT_06028314;
  *DAT_06028314 = param_2 & 0xff;
  FUN_06028018(puVar1,uVar2,1);
  return CONCAT44(param_2,param_1);
}

