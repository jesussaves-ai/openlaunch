// Function: FUN_00029890
// Address: 00029890


undefined4 FUN_00029890(undefined4 param_1,byte *param_2,int param_3)

{
  byte *pbVar1;
  byte *extraout_r1;
  
  do {
    pbVar1 = param_2 + 1;
    if (0x1f < *param_2) {
      FUN_000296b4();
      pbVar1 = extraout_r1;
    }
    param_3 = param_3 + -1;
    param_2 = pbVar1;
  } while (param_3 != 0);
  return param_1;
}

