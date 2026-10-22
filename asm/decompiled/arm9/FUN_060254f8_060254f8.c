// Function: FUN_060254f8
// Address:  060254f8
// Signature: undefined FUN_060254f8()


undefined8 FUN_060254f8(undefined4 param_1,undefined4 param_2,undefined4 param_3,uint param_4)

{
  uint uVar1;
  
  uVar1 = 0x18;
  do {
    FUN_06025484(param_4 >> (uVar1 & 0xff));
    uVar1 = uVar1 - 8;
  } while (-1 < (int)uVar1);
  return CONCAT44(param_2,param_1);
}

