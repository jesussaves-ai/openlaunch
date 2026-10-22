// Function: FUN_06027158
// Address:  06027158
// Signature: undefined FUN_06027158()


undefined8 FUN_06027158(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  char in_NG;
  char in_ZR;
  char in_CY;
  char in_OV;
  byte in_Q;
  
  uVar1 = FUN_06027184(param_1,(uint)(byte)(in_NG << 4 | in_ZR << 3 | in_CY << 2 | in_OV << 1 | in_Q
                                           ) << 0x1b | 0xc0);
  (*DAT_06027530)(uVar1,DAT_0602752c);
  FUN_06027184();
  return CONCAT44(param_2,param_1);
}

