// Function: FUN_0602b3f8
// Address:  0602b3f8
// Signature: undefined FUN_0602b3f8()


void FUN_0602b3f8(void)

{
  int unaff_r4;
  int unaff_r5;
  uint unaff_r7;
  uint unaff_r8;
  int unaff_r10;
  
  if ((unaff_r7 & 0x10) != 0) {
    if ((unaff_r8 & 1) == 0) {
      if ((unaff_r8 & 0x80000000) != 0) {
        FUN_0602c1a8(unaff_r4 + 0x610,unaff_r10,0x10);
      }
    }
    else {
      FUN_0602c1a8(unaff_r4 + 0x600,unaff_r10,0x10);
    }
    if ((unaff_r8 & 2) != 0) {
      FUN_0602c1a8(unaff_r4 + 0x580,unaff_r10 + 0x10,0x10);
    }
    if ((unaff_r8 & 4) != 0) {
      FUN_0602c1a8(unaff_r4 + 0x590,unaff_r10 + 0x20,0x10);
      FUN_0602c1a8(unaff_r4 + 0x5a0,unaff_r5 + 0xc0,0x10);
    }
  }
  return;
}

