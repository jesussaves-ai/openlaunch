// Function: FUN_06026910
// Address:  06026910
// Signature: undefined FUN_06026910()


undefined2 FUN_06026910(void)

{
  ushort extraout_r1;
  int unaff_r8;
  
  FUN_060268bc();
  *(ushort *)(unaff_r8 + 0x58) = extraout_r1 | 0x6000;
  FUN_060268bc();
  return *(undefined2 *)(unaff_r8 + 0x5c);
}

