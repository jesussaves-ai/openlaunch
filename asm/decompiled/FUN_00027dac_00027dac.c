// Function: FUN_00027dac
// Address: 00027dac


undefined4 FUN_00027dac(undefined4 param_1)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  
  uVar1 = FUN_00026bc8(param_1,0xc);
  uVar3 = uVar1 & 0xc;
  uVar1 = uVar1 & 0xf0;
  if (uVar3 == 0) {
                    /* WARNING: Could not recover jumptable at 0x00027e0c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    uVar2 = (**(code **)(DAT_000282a8 + (uVar1 >> 2)))();
    return uVar2;
  }
  if (uVar3 < 8) {
    if (0x9f < uVar1) {
      return 8;
    }
  }
  else if ((uVar3 == 8) && (uVar1 < 0x80)) {
    return 1;
  }
  return 0;
}

