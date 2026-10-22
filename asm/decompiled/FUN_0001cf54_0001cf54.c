// Function: FUN_0001cf54
// Address: 0001cf54


undefined8 FUN_0001cf54(uint param_1,undefined4 param_2)

{
  byte extraout_r1;
  int iVar1;
  uint uVar2;
  
  iVar1 = 8;
  param_1 = param_1 & 0xff;
  do {
    uVar2 = param_1 >> 1;
    DAT_04000138 = ((byte)param_1 & 1) + 0x74;
    FUN_0001cf14(5);
    DAT_04000138 = extraout_r1 | 2;
    FUN_0001cf14(5);
    param_1 = uVar2 | (uint)DAT_04000138 << 0x1f;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  return CONCAT44(param_2,param_1 >> 0x18);
}

