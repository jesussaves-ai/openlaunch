// Function: FUN_06025aa0
// Address:  06025aa0
// Signature: undefined FUN_06025aa0()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_06025aa0(uint param_1)

{
  do {
  } while ((_DAT_04000180 & 0xf) != param_1);
  _DAT_04000180 = param_1 << 8;
  return;
}

