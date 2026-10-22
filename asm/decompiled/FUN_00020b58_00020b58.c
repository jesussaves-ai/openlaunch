// Function: FUN_00020b58
// Address: 00020b58


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_00020b58(void)

{
  char in_NG;
  char in_ZR;
  char in_CY;
  char in_OV;
  byte in_Q;
  
  *DAT_00020cec = DAT_00020cf0;
  FUN_00020b04(0x40000);
  _DAT_04000208 = 1;
  return (uint)(byte)(in_NG << 4 | in_ZR << 3 | in_CY << 2 | in_OV << 1 | in_Q) << 0x1b;
}

