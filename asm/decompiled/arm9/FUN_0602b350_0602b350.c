// Function: FUN_0602b350
// Address:  0602b350
// Signature: undefined FUN_0602b350()


void FUN_0602b350(void)

{
  int unaff_r4;
  int unaff_r5;
  int unaff_r6;
  uint unaff_r7;
  uint unaff_r8;
  
  if (((unaff_r7 & 0x10) == 0) || ((DAT_0602b5cc != unaff_r6 && (DAT_0602b5d0 != unaff_r6)))) {
    if (((unaff_r7 & 1) == 0) || (DAT_0602b5d4 != unaff_r6)) {
      if (((unaff_r8 & 0x18) != 0) &&
         (FUN_0602c1a8(unaff_r4 + 0x5b0,unaff_r5 + 0x70,0x10), (unaff_r8 & 0x80) == 0)) {
        FUN_0602c1a8(unaff_r4 + 0x5c0,unaff_r5 + 0x70,0x10);
      }
    }
    else {
      FUN_0602c1a8(unaff_r4 + 0x5c0,unaff_r5 + 0x70,0x10);
      FUN_0602c1a8(unaff_r4 + 0x5b0,unaff_r5 + 0xa0,0x10);
    }
  }
  else {
    FUN_0602c1a8(unaff_r4 + 0x5b0,unaff_r5 + 0xa0,0x10);
  }
  return;
}

