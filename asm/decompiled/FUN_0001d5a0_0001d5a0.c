// Function: FUN_0001d5a0
// Address: 0001d5a0


void FUN_0001d5a0(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  uint *puVar1;
  undefined4 uVar2;
  
  uVar2 = DAT_0001dc0c;
  puVar1 = DAT_0001dc08;
  *DAT_0001dc08 = param_2 >> 8;
  FUN_0001d958(puVar1,uVar2,3,param_2 >> 8,param_2,param_3,param_4);
  uVar2 = DAT_0001dc14;
  puVar1 = DAT_0001dc10;
  *DAT_0001dc10 = param_2 & 0xff;
  uVar2 = FUN_0001d958(puVar1,uVar2,1);
  FUN_0001d65c(uVar2,DAT_0001dc18);
  return;
}

