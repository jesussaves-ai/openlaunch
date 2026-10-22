// Function: FUN_060264b8
// Address:  060264b8
// Signature: undefined FUN_060264b8()


void FUN_060264b8(undefined4 param_1,int param_2,int param_3)

{
  uint extraout_r1;
  int extraout_r1_00;
  uint uVar1;
  uint uVar2;
  
  uVar2 = param_2 + param_3 * 2;
  FUN_060264e4();
  uVar1 = extraout_r1;
  do {
    if (uVar2 <= uVar1) {
      return;
    }
    FUN_06026324(0x20);
    uVar1 = extraout_r1_00 + 2;
  } while (uVar1 < uVar2);
  return;
}

