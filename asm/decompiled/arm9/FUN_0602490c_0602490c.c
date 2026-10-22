// Function: FUN_0602490c
// Address:  0602490c
// Signature: undefined FUN_0602490c()


void FUN_0602490c(void)

{
  bool bVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  char *extraout_r1;
  char *unaff_r8;
  char *pcVar5;
  uint unaff_r10;
  uint uVar6;
  uint uVar7;
  undefined8 uVar8;
  
  uVar6 = 0;
  bVar1 = false;
  do {
    FUN_06026324(0xc);
    uVar2 = FUN_06026324(0xf);
    FUN_060264a0(uVar2,DAT_06025138);
LAB_06024930:
    do {
      FUN_06026324(0xe);
      uVar7 = 0;
      pcVar5 = DAT_0602513c;
      do {
        if (uVar6 == uVar7) {
          uVar2 = 0x13;
          unaff_r8 = pcVar5;
          unaff_r10 = uVar6;
        }
        else {
          uVar2 = 0x12;
        }
        FUN_06026324(uVar2);
        if (uVar6 == uVar7) {
          uVar2 = 0x23;
        }
        else {
          uVar2 = 0x20;
        }
        FUN_06026324(uVar2);
        uVar2 = FUN_06026480();
        FUN_060264a0(uVar2,pcVar5);
        FUN_06024aa0(uVar7);
        FUN_06026470();
        uVar7 = uVar7 + 1;
        pcVar5 = extraout_r1;
      } while (*extraout_r1 != '\0');
      FUN_06026470();
      uVar2 = FUN_06026324(0x14);
      iVar4 = unaff_r10 * 0x208 + DAT_06025140;
      if (5 < unaff_r10) {
        iVar4 = DAT_06025144;
      }
      FUN_060264b8(uVar2,iVar4,0x104);
      uVar7 = uVar7 - 1;
      do {
        uVar3 = FUN_060244e0();
        if ((uVar3 & 0x20000000) != 0) {
          FUN_060254d0(uVar3,0x40);
          do {
                    /* WARNING: Do nothing block with infinite loop */
          } while( true );
        }
        if ((uVar3 & 0xb) != 0) break;
        if ((uVar3 & 0x40) != 0) {
          if (uVar6 != 0) {
            uVar6 = uVar6 - 1;
          }
          goto LAB_06024930;
        }
        if ((uVar3 & 0x80) != 0) {
          if (uVar6 != uVar7) {
            uVar6 = uVar6 + 1;
          }
          goto LAB_06024930;
        }
        if ((uVar3 & 0x20) != 0) goto LAB_06024a90;
      } while ((uVar3 & 0x10) == 0);
    } while (unaff_r10 == 6);
    if (unaff_r10 == 7) {
LAB_06024a90:
      if (bVar1) {
        FUN_06024c90();
      }
      return;
    }
    FUN_0602c1a8(unaff_r8,DAT_06025148,0xc);
    uVar2 = 1;
    if (unaff_r10 == 5) {
      uVar2 = 2;
    }
    if (unaff_r10 == 0) {
      uVar2 = 3;
    }
    uVar8 = FUN_06024690(uVar2,DAT_0602514c);
    iVar4 = FUN_06028844();
    if (iVar4 != 0x55) {
      FUN_0602a4cc((int)uVar8,unaff_r10 * 0x208 + DAT_06025150);
      FUN_0602c1a8((int)((ulonglong)uVar8 >> 0x20),DAT_06025154 + unaff_r10 * 0x10,0xc);
      bVar1 = true;
    }
  } while( true );
}

