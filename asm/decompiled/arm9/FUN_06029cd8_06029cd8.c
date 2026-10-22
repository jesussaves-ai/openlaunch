// Function: FUN_06029cd8
// Address:  06029cd8
// Signature: undefined FUN_06029cd8()


undefined8 FUN_06029cd8(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined1 *extraout_r1;
  
  FUN_0602a3c4(DAT_0602a314);
  FUN_0602946c();
  FUN_06029f04(*DAT_0602a318,DAT_0602a31c,0x200);
  FUN_0602c1a8(DAT_0602a320,DAT_0602a324,0x128);
  FUN_0602a3c4(DAT_0602a328);
  FUN_0602946c();
  FUN_06029f00(*DAT_0602a32c,DAT_0602a330);
  FUN_0602c1a8(DAT_0602a334,DAT_0602a338,0x18);
  *DAT_0602a340 = *DAT_0602a33c;
  FUN_0602a3c4(DAT_0602a344);
  FUN_0602946c();
  FUN_06029f00(*DAT_0602a348,DAT_0602a34c);
  FUN_0602c1a8(DAT_0602a350,DAT_0602a354,0x14);
  FUN_0602a3c4(DAT_0602a358);
  FUN_0602946c();
  FUN_06029f04(*DAT_0602a35c,DAT_0602a360,0x200);
  uVar1 = FUN_0602bf6c(*(undefined4 *)(DAT_0602a364 + 0x1e4));
  FUN_06029e10(uVar1,DAT_0602a368);
  *extraout_r1 = 0;
  extraout_r1[1] = *(undefined1 *)(DAT_0602a36c + 0x193);
  return CONCAT44(param_2,param_1);
}

