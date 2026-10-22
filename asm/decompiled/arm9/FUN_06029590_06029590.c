// Function: FUN_06029590
// Address:  06029590
// Signature: undefined FUN_06029590()


undefined8 FUN_06029590(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  uint uVar2;
  
  if (DAT_060287ac == 0) {
    if (*DAT_06029860 < 0x10) {
      do {
                    /* WARNING: Do nothing block with infinite loop */
      } while( true );
    }
    if (*DAT_06029860 == 0x10) {
      iVar1 = DAT_06028780 + (DAT_060287a8 >> 8);
      if (iVar1 != *DAT_06029870) {
        *DAT_06029870 = iVar1;
        FUN_06029f04(iVar1,DAT_06029874,0x200);
      }
      uVar2 = (uint)*(ushort *)(DAT_06029878 + (DAT_060287a8 & 0xff) * 2);
      if (DAT_0602987c <= uVar2) {
        uVar2 = uVar2 | DAT_06029880;
      }
    }
    else {
      iVar1 = DAT_06028780 + (DAT_060287a8 >> 7);
      if (iVar1 != *DAT_06029864) {
        *DAT_06029864 = iVar1;
        FUN_06029f04(iVar1,DAT_06029868,0x200);
      }
      uVar2 = *(uint *)(DAT_0602986c + (DAT_060287a8 & 0x7f) * 4) & 0xfffffff;
    }
    FUN_06029474(uVar2);
  }
  return CONCAT44(param_2,param_1);
}

