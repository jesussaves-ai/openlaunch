// Function: FUN_00021fa4
// Address: 00021fa4


undefined4 FUN_00021fa4(ushort *param_1,int param_2)

{
  undefined4 uVar1;
  
  if (param_2 != 0) {
    if ((param_1 != (ushort *)0x0) && ((*param_1 & 0x8000) != 0)) {
                    /* WARNING: Could not recover jumptable at 0x00021fc0. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      uVar1 = (**(code **)(param_1 + 4))();
      return uVar1;
    }
    FUN_000223c8(param_2);
  }
  return 0;
}

