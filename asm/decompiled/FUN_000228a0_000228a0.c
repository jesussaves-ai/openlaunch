// Function: FUN_000228a0
// Address: 000228a0


undefined4 FUN_000228a0(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  
  uVar1 = FUN_00020ae4();
  if (param_1 == (uint *)0x0) {
LAB_000228d8:
    uVar3 = FUN_0001cd28();
  }
  else {
    if ((*param_1 & 0x8000) == 0) {
      uVar3 = 0;
      goto LAB_000228f4;
    }
    uVar2 = *param_1 & 1;
    if (uVar2 == 1) {
      FUN_000234a4(param_1[1]);
    }
    else {
      if (uVar2 != 0) goto LAB_000228d8;
      FUN_00024680(param_1[1]);
    }
    uVar3 = 0;
    param_1[1] = 0;
    *param_1 = 0;
  }
LAB_000228f4:
  FUN_00020af8(uVar1);
  return uVar3;
}

