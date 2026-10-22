// Function: FUN_00028198
// Address: 00028198


void FUN_00028198(undefined4 param_1)

{
  uint uVar1;
  undefined1 auStack_224 [10];
  undefined1 auStack_21a [6];
  undefined1 auStack_214 [8];
  undefined1 auStack_20c [488];
  undefined1 auStack_24 [6];
  undefined1 auStack_1e [6];
  undefined1 local_18;
  
  uVar1 = FUN_00026bc8(param_1,8);
  if (0x200 < uVar1) {
    uVar1 = 0x200;
  }
  local_18 = FUN_00026bc8(param_1,10);
  FUN_00026be0(auStack_224,param_1,0xc,uVar1 + 1 & 0xfffffffe);
  FUN_00025f60(auStack_21a,auStack_24);
  FUN_00025f60(auStack_214,auStack_1e);
  if (0x18 < uVar1) {
    FUN_00025fac(auStack_24,auStack_20c);
  }
  return;
}

