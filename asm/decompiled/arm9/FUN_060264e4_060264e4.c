// Function: FUN_060264e4
// Address:  060264e4
// Signature: undefined FUN_060264e4()


undefined4 FUN_060264e4(undefined4 param_1,short *param_2,int param_3)

{
  short sVar1;
  short *extraout_r1;
  
  while( true ) {
    param_3 = param_3 + -1;
    if (param_3 < 0) {
      return param_1;
    }
    sVar1 = *param_2;
    if (sVar1 == 0) break;
    if (sVar1 == 10) {
      return param_1;
    }
    FUN_06026324(sVar1,param_2 + 1);
    param_2 = extraout_r1;
  }
  return param_1;
}

