// Function: FUN_0001ca10
// Address: 0001ca10


undefined8 FUN_0001ca10(undefined4 param_1,undefined4 param_2,undefined4 param_3,uint param_4)

{
  uint uVar1;
  
  uVar1 = 0x18;
  do {
    FUN_0001c984(param_4 >> (uVar1 & 0xff));
    uVar1 = uVar1 - 8;
  } while (-1 < (int)uVar1);
  return CONCAT44(param_2,param_1);
}

