// Function: FUN_06025b9c
// Address:  06025b9c
// Signature: undefined FUN_06025b9c()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_06025b9c(uint param_1)

{
  while ((_DAT_04000180 & 0xf) != param_1) {
    if ((_DAT_04000208 & 1) != 0) {
      software_interrupt(0x60000);
    }
  }
  _DAT_04000180 = param_1 << 8 | 0x6000;
  return;
}

