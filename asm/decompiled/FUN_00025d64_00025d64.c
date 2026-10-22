// Function: FUN_00025d64
// Address: 00025d64


undefined4
FUN_00025d64(undefined4 *param_1,undefined4 *param_2,undefined4 *param_3,undefined4 *param_4)

{
  undefined4 uVar1;
  int iVar2;
  
  iVar2 = *DAT_00025e88;
  if (iVar2 == 0) {
    uVar1 = 0xffffffff;
  }
  else {
    if (param_1 != (undefined4 *)0x0) {
      *param_1 = *(undefined4 *)(iVar2 + 0x14);
    }
    if (param_2 != (undefined4 *)0x0) {
      *param_2 = *(undefined4 *)(iVar2 + 0x18);
    }
    if (param_3 != (undefined4 *)0x0) {
      *param_3 = *(undefined4 *)(iVar2 + 0x1c);
    }
    if (param_4 != (undefined4 *)0x0) {
      *param_4 = *(undefined4 *)(iVar2 + 0x20);
    }
    uVar1 = *(undefined4 *)(iVar2 + 0x10);
  }
  return uVar1;
}

