// Function: FUN_0001d5e4
// Address: 0001d5e4


undefined8 FUN_0001d5e4(undefined4 param_1,uint param_2)

{
  uint *puVar1;
  undefined4 uVar2;
  
  FUN_0001d6a0(param_1,DAT_0001dc1c);
  uVar2 = DAT_0001dc24;
  puVar1 = DAT_0001dc20;
  *DAT_0001dc20 = param_2 >> 8;
  FUN_0001d958(puVar1,uVar2,3);
  uVar2 = DAT_0001dc2c;
  puVar1 = DAT_0001dc28;
  *DAT_0001dc28 = param_2 & 0xff;
  FUN_0001d958(puVar1,uVar2,1);
  return CONCAT44(param_2,param_1);
}

