// Function: FUN_00021754
// Address: 00021754


void FUN_00021754(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 local_24;
  undefined1 auStack_20 [2];
  undefined2 local_1e;
  undefined4 local_1c;
  
  FUN_00021c04();
  *DAT_00021c80 = param_1;
  *DAT_00021c84 = param_2;
  uVar1 = *DAT_00021c88;
  *DAT_00021c8c = uVar1;
  *DAT_00021c90 = uVar1;
  *DAT_00021c94 = uVar1;
  *DAT_00021c98 = 1;
  *DAT_00021c9c = 0;
  *DAT_00021ca0 = 0;
  *DAT_00021ca4 = 0;
  *DAT_00021ca8 = 0;
  *DAT_00021cac = 0;
  *DAT_00021cb0 = 0;
  *DAT_00021cb4 = 0;
  local_1e = 0x4400;
  local_1c = 0;
  uVar1 = FUN_00022764(2,2,0);
  *DAT_00021cb8 = uVar1;
  FUN_000229e4(uVar1,auStack_20,0x10);
  local_24 = 1;
  uVar1 = FUN_00022f70(*DAT_00021cbc,1,&local_24);
  FUN_00029878(uVar1,DAT_00021cc0);
  FUN_00021558(1);
  return;
}

