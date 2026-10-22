// Function: FUN_00025904
// Address: 00025904


undefined4 FUN_00025904(int param_1)

{
  undefined4 uVar1;
  
  *(undefined4 *)(param_1 + 4) = DAT_00025cb8;
  *(undefined4 *)(param_1 + 0x10) = DAT_00025cbc;
  *(undefined4 *)(param_1 + 0x18) = DAT_00025cc0;
  uVar1 = DAT_00025cc4;
  *(undefined4 *)(param_1 + 0x14) = DAT_00025cc4;
  *(undefined4 *)(param_1 + 0x1c) = uVar1;
  *(undefined2 *)(param_1 + 2) = 6;
  FUN_00024aa4(DAT_00025cc8,param_1 + 0x2c);
  *(undefined4 *)(param_1 + 0xc) = 0;
  return 0;
}

