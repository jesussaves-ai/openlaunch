// Function: FUN_06025c20
// Address:  06025c20
// Signature: undefined FUN_06025c20()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_06025c20(void)

{
  uint uVar1;
  char in_NG;
  char in_ZR;
  char in_CY;
  char in_OV;
  byte in_Q;
  undefined4 in_cr1;
  undefined4 in_cr9;
  
  uVar1 = coprocessor_movefromRt(0xf,0,0,in_cr9,in_cr1);
  *(undefined4 *)((uVar1 & 0xfffff000) + 0x3ffc) = DAT_06025de8;
  _DAT_04000208 = 1;
  return (uint)(byte)(in_NG << 4 | in_ZR << 3 | in_CY << 2 | in_OV << 1 | in_Q) << 0x1b;
}

