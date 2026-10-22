// Function: FUN_0001ee10
// Address: 0001ee10


void FUN_0001ee10(int param_1,undefined4 param_2,int param_3)

{
  *(byte *)(param_1 + 0x24) = *(byte *)(param_1 + 0x24) & 0xf8 | *DAT_0001f7fc;
  FUN_00025f60(DAT_0001f800,param_1 + 8);
  FUN_00025f60(DAT_0001f804,param_1 + 0xe);
  FUN_0001c7d0(param_2,param_1 + 0x27,8);
  FUN_0001c72c(param_1 + 0x2f,0x20);
  if (param_3 != 0) {
    FUN_0001c7d0(param_3,param_1 + 0x2f,0x20);
  }
  FUN_0001ff1c(param_1 + 0x1e);
  FUN_0001f1b4(param_1);
  return;
}

