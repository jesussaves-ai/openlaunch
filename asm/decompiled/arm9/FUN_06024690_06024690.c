// Function: FUN_06024690
// Address:  06024690
// Signature: undefined FUN_06024690()


undefined8 FUN_06024690(int param_1,undefined4 param_2)

{
  byte *pbVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  ushort *puVar6;
  ushort *unaff_r10;
  uint uVar7;
  uint uVar8;
  undefined8 uVar9;
  
  *DAT_0602510c = param_1;
LAB_060246a4:
  FUN_06028960();
  FUN_06026324(0xc);
  uVar2 = FUN_06026324(0xf);
  uVar2 = FUN_060264a0(uVar2,param_2);
  if (param_1 == 0) {
    uVar2 = 0x37;
  }
  if (param_1 == 1) {
    uVar2 = 0xff;
  }
  if (param_1 == 2) {
    uVar2 = 0x79;
  }
  if (param_1 == 3) {
    uVar2 = 0x7f;
  }
  FUN_0602a72c(uVar2);
  uVar7 = 0;
  iVar5 = 0;
LAB_060246ec:
  do {
    FUN_06026324(0xe);
    puVar6 = (ushort *)*DAT_06025110;
    for (iVar4 = iVar5; iVar4 != 0; iVar4 = iVar4 + -1) {
      do {
        pbVar1 = (byte *)((int)puVar6 + 3);
        puVar6 = (ushort *)((int)puVar6 + (uint)*puVar6);
      } while ((*pbVar1 & 0x10) != 0);
    }
    uVar8 = 0;
    for (; puVar6 < (ushort *)*DAT_06025114; puVar6 = (ushort *)((int)puVar6 + (uint)*puVar6)) {
      if ((*(byte *)((int)puVar6 + 3) & 0x10) == 0) {
        if (0x12 < uVar8) break;
        FUN_0602aa84(puVar6);
        FUN_0602ac1c(puVar6);
        uVar3 = FUN_06028844(puVar6 + 0x1a);
        uVar2 = 0x12;
        if (0x54 < uVar3) {
          uVar2 = 0x15;
        }
        if (uVar3 == 0x43) {
          uVar2 = 0x16;
        }
        if (uVar7 == uVar8) {
          uVar2 = 0x13;
          unaff_r10 = puVar6;
        }
        FUN_06026324(uVar2);
        if (uVar7 == uVar8) {
          uVar2 = 0x23;
        }
        else {
          uVar2 = 0x20;
        }
        FUN_06026324(uVar2);
        uVar2 = FUN_06026480();
        if (*(int *)(puVar6 + 0x18) == 0) {
          uVar9 = FUN_0602650c(uVar2,puVar6 + 6,0xc);
          FUN_06026534((int)uVar9,(int)((ulonglong)uVar9 >> 0x20),0x11);
        }
        else {
          iVar4 = *(int *)(puVar6 + 0x18) + 0x340;
          if ((*(uint *)(puVar6 + 0xe) & 0xffffff00) == DAT_06025118 &&
              *(int *)(puVar6 + 0x10) == DAT_0602511c) {
            iVar4 = DAT_06025120;
          }
          FUN_060264b8(*(int *)(puVar6 + 0x10),iVar4,0x1d);
        }
        FUN_06026470();
        uVar8 = uVar8 + 1;
      }
    }
    FUN_06026324(10);
    uVar2 = FUN_06026324(0x14);
    FUN_060264a0(uVar2,DAT_06025124);
    FUN_060265e0(*(undefined4 *)(unaff_r10 + 0x12));
    uVar2 = FUN_06026470();
    FUN_060264a0(uVar2,DAT_06025128);
    FUN_060265e0(*(undefined4 *)(unaff_r10 + 0x14));
    uVar2 = FUN_06026470();
    FUN_060264b8(uVar2,unaff_r10 + 0x1a);
    uVar8 = uVar8 - 1;
    while( true ) {
      uVar3 = FUN_060244e0();
      if ((uVar3 & 0x40000000) != 0) goto LAB_060246a4;
      if ((uVar3 & 0x80000000) != 0) {
        *(byte *)(*DAT_0602512c + 2) = *(byte *)(*DAT_0602512c + 2) & 0xfe;
        goto LAB_060246ec;
      }
      if ((uVar3 & 0x20000000) != 0) {
        FUN_060254d0(uVar3,0x40);
        do {
                    /* WARNING: Do nothing block with infinite loop */
        } while( true );
      }
      if ((uVar3 & 0xb) != 0) goto LAB_060248f4;
      if ((uVar3 & 0x40) != 0) {
        if (uVar7 == 0) {
          if (iVar5 != 0) {
            iVar5 = iVar5 + -1;
          }
        }
        else {
          uVar7 = uVar7 - 1;
        }
        goto LAB_060246ec;
      }
      if ((uVar3 & 0x80) != 0) break;
      if ((uVar3 & 0x20) == 0) {
        if ((uVar3 & 0x10) != 0) goto LAB_060248f4;
      }
      else if (param_1 != 0) {
        unaff_r10 = (ushort *)*DAT_06025134;
LAB_060248f4:
        FUN_06026324(0xc);
        return CONCAT44(unaff_r10 + 6,unaff_r10 + 0x1a);
      }
    }
    if (uVar7 == uVar8) {
      if (puVar6 < (ushort *)*DAT_06025130) {
        iVar5 = iVar5 + 1;
      }
    }
    else {
      uVar7 = uVar7 + 1;
    }
  } while( true );
}

