// Function: FUN_0001d7f8
// Address: 0001d7f8


void FUN_0001d7f8(undefined4 param_1,undefined4 param_2)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_0001d6b8();
  uVar2 = DAT_0001dc80;
  puVar1 = DAT_0001dc7c;
  *DAT_0001dc7c = 7;
  puVar1[1] = param_1;
  puVar1[2] = param_2;
  FUN_0001d958(puVar1,uVar2,0xc);
  return;
}

