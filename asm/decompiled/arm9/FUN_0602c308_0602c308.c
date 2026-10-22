// Function: FUN_0602c308
// Address:  0602c308
// Signature: undefined FUN_0602c308()


undefined8 FUN_0602c308(uint param_1,uint param_2,undefined4 param_3,uint *param_4)

{
  uint uVar1;
  
  uVar1 = *param_4;
  if (param_1 < uVar1) {
    if (uVar1 < param_2) {
      param_2 = uVar1;
    }
    return CONCAT44(param_2,param_1);
  }
  if (param_1 < uVar1 + param_4[1]) {
    param_1 = uVar1 + param_4[1];
  }
  return CONCAT44(param_2,param_1);
}

