// Function: FUN_0001ef00
// Address: 0001ef00


void FUN_0001ef00(int param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  
  iVar1 = DAT_0001f82c;
  FUN_0001c7d0(param_1,DAT_0001f82c + 0x14,0x10);
  FUN_0001c7d0(param_1 + 0x18,extraout_r1,8);
  FUN_0001c7d0(param_1 + 0x10,extraout_r1_00,8);
  FUN_0001c72c(iVar1 + 0xc,8);
  if (param_2 != 0) {
    FUN_0001c7d0(param_2,iVar1 + 0xc,8);
  }
  *(undefined4 *)(iVar1 + 8) = param_3;
  FUN_0001f200(iVar1);
  return;
}

