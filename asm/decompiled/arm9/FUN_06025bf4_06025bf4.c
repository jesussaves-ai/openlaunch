// Function: FUN_06025bf4
// Address:  06025bf4
// Signature: undefined FUN_06025bf4()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_06025bf4(void)

{
  char in_NG;
  char in_ZR;
  char in_CY;
  char in_OV;
  byte in_Q;
  
  *DAT_06025de0 = DAT_06025de4;
  _DAT_04000208 = 1;
  return (uint)(byte)(in_NG << 4 | in_ZR << 3 | in_CY << 2 | in_OV << 1 | in_Q) << 0x1b;
}

