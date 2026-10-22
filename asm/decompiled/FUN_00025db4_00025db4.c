// Function: FUN_00025db4
// Address: 00025db4


void FUN_00025db4(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 unaff_r4;
  int iVar1;
  
  iVar1 = *DAT_00025e8c;
  if (iVar1 != 0) {
    *(undefined4 *)(iVar1 + 0x10) = param_1;
    *(undefined4 *)(iVar1 + 0x14) = param_2;
    *(undefined4 *)(iVar1 + 0x18) = param_3;
    *(undefined4 *)(iVar1 + 0x1c) = param_4;
    *(undefined4 *)(iVar1 + 0x20) = unaff_r4;
    FUN_00020f8c(iVar1);
  }
  return;
}

