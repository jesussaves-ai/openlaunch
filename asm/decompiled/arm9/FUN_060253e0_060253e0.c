// Function: FUN_060253e0
// Address:  060253e0
// Signature: undefined FUN_060253e0()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_060253e0(int param_1)

{
  _DAT_04000180 = param_1 << 8 | 0x6000;
  while (param_1 != 0) {
    if ((_DAT_04000208 & 1) != 0) {
      software_interrupt(0x60000);
    }
  }
  return;
}

