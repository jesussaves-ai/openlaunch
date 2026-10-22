// Function: FUN_0001e934
// Address: 0001e934


void FUN_0001e934(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined4 uVar6;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  undefined4 extraout_r1_01;
  undefined4 extraout_r1_02;
  undefined1 *extraout_r1_03;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined1 uVar9;
  bool bVar10;
  undefined1 uVar11;
  undefined1 auStack_8c [100];
  
  iVar3 = DAT_0001f6f8;
  uVar5 = (uint)CONCAT11(*(undefined1 *)(DAT_0001f6f8 + 0x23),*(undefined1 *)(DAT_0001f6f8 + 0x24));
  if (*(char *)(DAT_0001f6f8 + 0x22) == -2) {
    if (*DAT_0001f6fc != '\x01') goto LAB_0001e9d4;
    uVar5 = uVar5 & 0xfffffff8;
    if (uVar5 == 0x88) goto LAB_0001e9fc;
    if (uVar5 == DAT_0001f700) goto LAB_0001eb34;
    if ((uVar5 != DAT_0001f704) && (uVar5 != DAT_0001f708)) goto LAB_0001e9d4;
  }
  else {
    if ((*(char *)(DAT_0001f6f8 + 0x22) != '\x02') || (*DAT_0001f6fc != '\x02')) goto LAB_0001e9d4;
    uVar5 = uVar5 & 0xfffffff8;
    if (uVar5 == 0x88) {
LAB_0001e9fc:
      cVar1 = *DAT_0001f718;
      if (cVar1 == '\x03') {
LAB_0001eb14:
        uVar6 = DAT_0001f750;
        if (*DAT_0001f74c != '\x01') {
          uVar6 = DAT_0001f754;
        }
        *DAT_0001f758 = uVar6;
        return;
      }
      uVar11 = cVar1 != '\0';
      uVar9 = cVar1 == '\x01';
      if ((bool)uVar9) {
        *DAT_0001f71c = *(byte *)(DAT_0001f6f8 + 0x24) & 7;
        FUN_0001c7d0(iVar3 + 0x27,DAT_0001f720,8);
        FUN_0001c7d0(iVar3 + 0x2f,DAT_0001f724,0x20);
        FUN_0001d030(DAT_0001f728,0x20);
        uVar8 = DAT_0001f730;
        uVar6 = DAT_0001f72c;
        FUN_0001caa8(DAT_0001f72c,DAT_0001f730,6);
        uVar4 = DAT_0001f738;
        uVar2 = DAT_0001f734;
        uVar7 = uVar8;
        if ((bool)uVar11 && !(bool)uVar9) {
          uVar7 = uVar6;
          uVar6 = uVar8;
        }
        FUN_0001caa8(DAT_0001f734,DAT_0001f738,0x20);
        uVar8 = uVar4;
        if ((bool)uVar11 && !(bool)uVar9) {
          uVar8 = uVar2;
          uVar2 = uVar4;
        }
        FUN_0001c7d0(DAT_0001f73c,auStack_8c,0x17);
        FUN_0001c7d0(uVar6,extraout_r1,6);
        FUN_0001c7d0(uVar7,extraout_r1_00,6);
        FUN_0001c7d0(uVar2,extraout_r1_01,0x20);
        FUN_0001c7d0(uVar8,extraout_r1_02,0x20);
        *extraout_r1_03 = 0;
        FUN_0001f9c8(auStack_8c,100,DAT_0001f740,0x20);
        *DAT_0001f748 = 3;
        goto LAB_0001eb14;
      }
      goto LAB_0001e9d4;
    }
    if (uVar5 == DAT_0001f70c) {
LAB_0001eb34:
      bVar10 = true;
      FUN_0001ff8c(DAT_0001f6f8 + 0x1e);
      if ((!bVar10) || (FUN_0001caa8(iVar3 + 0x2f,DAT_0001f75c,0x20), !bVar10)) {
LAB_0001ebdc:
        FUN_000296b4(0x42);
        FUN_000296b4(0x41);
        FUN_000296b4(0x44);
        FUN_000296b4(0x23);
        FUN_000296b4(0x33);
        return;
      }
      if (*DAT_0001f760 == '\x05') {
LAB_0001ebbc:
        uVar6 = DAT_0001f774;
        if (*DAT_0001f770 != '\x01') {
          uVar6 = DAT_0001f778;
        }
        *DAT_0001f77c = uVar6;
        return;
      }
      bVar10 = *DAT_0001f760 == '\x03';
      if ((bVar10) && (FUN_0001fec4(iVar3 + 0x1e), bVar10)) {
        uVar9 = *DAT_0001f764 == '\x02';
        if ((bool)uVar9) {
          FUN_0001c7d0(iVar3 + 0x5f,DAT_0001f768,8);
          FUN_0001ffc8(iVar3 + 0x1e);
          if ((!(bool)uVar9) || (FUN_0002013c(iVar3 + 0x1e), !(bool)uVar9)) goto LAB_0001ebdc;
        }
        *DAT_0001f76c = 5;
        goto LAB_0001ebbc;
      }
      goto LAB_0001e9d4;
    }
    if (uVar5 != DAT_0001f710) goto LAB_0001e9d4;
  }
  uVar9 = *DAT_0001f780 == '\x05';
  if (!(bool)uVar9) {
LAB_0001e9d4:
    FUN_000298e0();
    FUN_00029840();
    FUN_000296b4(0x23);
    FUN_000296b4(0x3f);
    *DAT_0001f714 = 0;
    return;
  }
  FUN_0001ff8c(DAT_0001f6f8 + 0x1e);
  if ((bool)uVar9) {
    FUN_0001fec4(iVar3 + 0x1e);
    FUN_0001c7d0(iVar3 + 0x5f,DAT_0001f784,8);
    FUN_0001ffc8(iVar3 + 0x1e);
    if ((bool)uVar9) {
      bVar10 = *DAT_0001f788 == '\x01';
      if (bVar10) {
        FUN_000200d8(iVar3 + 0x1e);
        if (bVar10) goto LAB_0001ec74;
      }
      else {
        FUN_0002013c(iVar3 + 0x1e);
        if (bVar10) {
LAB_0001ec74:
          uVar6 = DAT_0001f790;
          if (*DAT_0001f78c != '\x01') {
            uVar6 = DAT_0001f794;
          }
          *DAT_0001f798 = uVar6;
          return;
        }
      }
    }
  }
  FUN_000296b4(0x42);
  FUN_000296b4(0x41);
  FUN_000296b4(0x44);
  FUN_000296b4(0x23);
  FUN_000296b4(0x35);
  return;
}

