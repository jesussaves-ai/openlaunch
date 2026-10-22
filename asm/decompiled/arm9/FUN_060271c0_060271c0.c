// Function: FUN_060271c0
// Address:  060271c0
// Signature: undefined FUN_060271c0()


undefined8 FUN_060271c0(undefined4 param_1,undefined4 param_2)

{
  if (((*(byte *)(DAT_0602753c + 0x12) & 2) != 0) && ((*(byte *)(DAT_0602753c + 0x1c) & 2) != 0)) {
    if (*DAT_06027540 != '\0') {
      FUN_06027370();
      FUN_06027210();
      FUN_06027210();
    }
  }
  return CONCAT44(param_2,param_1);
}

