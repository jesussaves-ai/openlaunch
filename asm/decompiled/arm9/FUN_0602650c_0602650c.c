// Function: FUN_0602650c
// Address:  0602650c
// Signature: undefined FUN_0602650c()


undefined4 FUN_0602650c(undefined4 param_1,byte *param_2,int param_3)

{
  byte bVar1;
  byte *extraout_r1;
  
  do {
    bVar1 = *param_2;
    if (bVar1 == 0) {
      bVar1 = 0x20;
    }
    if (bVar1 < 0x20) {
      bVar1 = 0x3f;
    }
    FUN_06026324(bVar1,param_2 + 1);
    param_3 = param_3 + -1;
    param_2 = extraout_r1;
  } while (param_3 != 0);
  return param_1;
}

