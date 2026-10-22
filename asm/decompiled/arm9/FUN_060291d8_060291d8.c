// Function: FUN_060291d8
// Address:  060291d8
// Signature: undefined FUN_060291d8()


ulonglong FUN_060291d8(undefined4 param_1,undefined4 param_2)

{
  byte bVar1;
  byte *pbVar2;
  undefined4 uVar3;
  ushort *puVar4;
  ushort *puVar5;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  undefined2 *extraout_r1_01;
  int iVar6;
  uint uVar7;
  uint uVar8;
  byte *unaff_r11;
  undefined1 in_ZR;
  undefined1 uVar9;
  
  FUN_0602946c();
  uVar9 = 0;
  if ((bool)in_ZR) {
LAB_060293f4:
    uVar3 = 0;
LAB_060293f8:
    return (ulonglong)CONCAT14(unaff_r11[0xb],uVar3);
  }
LAB_060291e8:
  do {
    *DAT_060297f0 = 0xff;
    while( true ) {
      FUN_06029404();
      if (((bool)uVar9) || (*unaff_r11 == 0)) goto LAB_060293f4;
      uVar9 = 1;
      if (*unaff_r11 == 0xe5) goto LAB_060291e8;
      if (unaff_r11[0xb] != 0xf) break;
      uVar7 = (uint)*unaff_r11;
      bVar1 = unaff_r11[0xd];
      if ((*unaff_r11 & 0x40) != 0) {
        uVar7 = uVar7 & 0x3f;
        *DAT_06029828 = (char)uVar7;
        *DAT_0602982c = bVar1;
      }
      uVar9 = *DAT_06029830 == uVar7;
      if ((!(bool)uVar9) || (uVar9 = *DAT_06029834 == bVar1, !(bool)uVar9)) goto LAB_060291e8;
      uVar7 = (uVar7 & 0x3f) - 1;
      uVar9 = uVar7 == 0x14;
      if (0x13 < uVar7) goto LAB_060291e8;
      FUN_0602c1a8(unaff_r11 + 1,uVar7 * 0x1a + DAT_06029838,10);
      FUN_0602c1a8(unaff_r11 + 0xe,extraout_r1,0xc);
      FUN_0602c1a8(unaff_r11 + 0x1c,extraout_r1_00,4);
      uVar9 = (*unaff_r11 & 0x40) == 0;
      if (!(bool)uVar9) {
        *extraout_r1_01 = 0;
      }
      *DAT_0602983c = *DAT_0602983c + -1;
    }
    iVar6 = 0xb;
    pbVar2 = unaff_r11;
    puVar5 = DAT_060297f4;
    do {
      puVar4 = puVar5;
      if (iVar6 == 3) {
        puVar4 = puVar5 + 1;
        *puVar5 = 0x2e;
      }
      puVar5 = puVar4;
      if (*pbVar2 != 0x20) {
        puVar5 = puVar4 + 1;
        *puVar4 = (ushort)*pbVar2;
      }
      iVar6 = iVar6 + -1;
      pbVar2 = pbVar2 + 1;
    } while (iVar6 != 0);
    if (puVar5[-1] == 0x2e) {
      puVar5 = puVar5 + -1;
    }
    *puVar5 = 0;
    iVar6 = 0xb;
    uVar7 = 0;
    pbVar2 = unaff_r11;
    do {
      uVar8 = uVar7 >> 1;
      if ((bool)((byte)uVar7 & 1)) {
        uVar8 = uVar8 | 0x80;
      }
      uVar7 = uVar8 + *pbVar2 & 0xff;
      iVar6 = iVar6 + -1;
      pbVar2 = pbVar2 + 1;
    } while (iVar6 != 0);
    if ((*DAT_060297f8 != uVar7) || (*DAT_060297fc != '\0')) {
      *DAT_06029800 = 0xff;
      FUN_0602c1a8(DAT_06029804,DAT_06029808,0x1a);
    }
    *DAT_0602980c = (uint)unaff_r11[0xb];
    *DAT_06029810 = *(undefined4 *)(unaff_r11 + 0x1c);
    uVar7 = (uint)*(ushort *)(unaff_r11 + 0x1a);
    if (DAT_06028516 == ' ') {
      uVar7 = CONCAT22(*(undefined2 *)(unaff_r11 + 0x14),*(ushort *)(unaff_r11 + 0x1a));
    }
    *DAT_06029814 = uVar7;
    uVar9 = *DAT_06029818 == 0;
    if ((bool)uVar9) {
      FUN_0602a4f4(param_2,DAT_0602981c);
      if (((bool)uVar9) || (FUN_0602a4f4(param_2,DAT_06029820), (bool)uVar9)) {
        uVar3 = *DAT_06029824;
        goto LAB_060293f8;
      }
      uVar9 = 0;
    }
    else {
      FUN_06029400();
    }
  } while( true );
}

