// Function: FUN_00029850
// Address: 00029850


undefined4 FUN_00029850(undefined4 param_1,byte *param_2)

{
  byte bVar1;
  byte *extraout_r1;
  
  while( true ) {
    bVar1 = *param_2;
    if (bVar1 == 0) break;
    if (bVar1 < 0x20) {
      bVar1 = 0x3f;
    }
    if (0x7f < bVar1) {
      bVar1 = 0x3f;
    }
    FUN_000296b4(bVar1,param_2 + 1);
    param_2 = extraout_r1;
  }
  return param_1;
}

