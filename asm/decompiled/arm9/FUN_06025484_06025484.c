// Function: FUN_06025484
// Address:  06025484
// Signature: undefined FUN_06025484()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 FUN_06025484(int param_1,undefined4 param_2,int param_3)

{
  undefined1 uVar1;
  uint uVar2;
  
  uVar2 = 0x8100;
  if (param_3 != 1) {
    uVar2 = 0x8900;
  }
  _DAT_040001c0 = uVar2 | param_1 << 0x10;
  uVar2 = _DAT_040001c0;
  if (param_3 == 1) {
    software_interrupt(0x30000);
  }
  DAT_040001c2 = (undefined1)param_1;
  DAT_04000208 = param_3 == 1;
  uVar1 = DAT_040001c2;
  _DAT_040001c0 = uVar2;
  return uVar1;
}

