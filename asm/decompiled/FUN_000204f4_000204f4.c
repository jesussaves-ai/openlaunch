// Function: FUN_000204f4
// Address: 000204f4


void FUN_000204f4(undefined4 param_1,undefined4 param_2,undefined1 *param_3,uint param_4)

{
  undefined1 auStack_d8 [64];
  undefined1 auStack_98 [120];
  
  if (0x40 < param_4) {
    param_4 = FUN_0001fdc8(auStack_d8,param_3,param_4);
    param_3 = auStack_d8;
  }
  FUN_0001c7d0(param_3,auStack_d8,param_4);
  FUN_0001c72c(auStack_d8 + param_4,0x40 - param_4);
  FUN_00020578(0x36,auStack_98);
  FUN_00020578(0x6a);
  return;
}

