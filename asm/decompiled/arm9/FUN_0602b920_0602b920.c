// Function: FUN_0602b920
// Address:  0602b920
// Signature: undefined FUN_0602b920()


void FUN_0602b920(int param_1)

{
  undefined4 uVar1;
  int unaff_r4;
  
  if (param_1 == 0) {
    uVar1 = 0x3c000000;
  }
  else {
    uVar1 = 0x3d000000;
  }
  FUN_0602bd94(uVar1,0,0,DAT_0602bee8);
  *DAT_0602beec = 0;
  FUN_06026c50(*(undefined4 *)(DAT_0602bef0 + 0xc),8);
  *(uint *)(unaff_r4 + 0x1a4) =
       (DAT_0602bef8 & *(uint *)(DAT_0602bef4 + 100)) +
       ((*(uint *)(DAT_0602bef4 + 100) & 0x3f0000) >> 0x10) | 0x30000000;
  FUN_0602bce4(0x40000000,0,0,DAT_0602befc);
  uVar1 = DAT_0602bf08;
  *(uint *)(unaff_r4 + 0x1b0) =
       *(byte *)(DAT_0602bf04 + (*(byte *)(DAT_0602bf00 + 0x13) & 7)) | 0x6000;
  *(undefined4 *)(unaff_r4 + 0x1b4) = uVar1;
  *(undefined4 *)(unaff_r4 + 0x1b8) = 0x5c0000;
  *(uint *)(unaff_r4 + 0x1a4) = *(uint *)(unaff_r4 + 0x1a4) | 0xe000;
  return;
}

