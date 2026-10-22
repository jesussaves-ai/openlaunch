// Function: FUN_06028cdc
// Address:  06028cdc
// Signature: undefined FUN_06028cdc()


undefined8 FUN_06028cdc(undefined4 param_1,undefined4 param_2)

{
  undefined2 uVar1;
  uint uVar2;
  undefined4 uVar3;
  int extraout_r1;
  int iVar4;
  int extraout_r1_00;
  int extraout_r1_01;
  int extraout_r1_02;
  undefined4 extraout_r1_03;
  int extraout_r1_04;
  int extraout_r1_05;
  undefined1 in_ZR;
  undefined1 uVar5;
  undefined1 uVar6;
  undefined8 uVar7;
  
  FUN_06028cc8();
  uVar5 = true;
  if (((bool)in_ZR) || (uVar5 = (*(uint *)(DAT_06029754 + 0x1c) & 0x20) == 0, !(bool)uVar5)) {
    FUN_060287b8(0,0);
    FUN_06028cb4();
    if (!(bool)uVar5) {
      FUN_060287b8(DAT_06029758,0x408);
    }
    do {
      FUN_06028cb4();
      if ((bool)uVar5) {
LAB_06028d50:
        FUN_060287b8(0x100000,1);
      }
      else {
        FUN_060287b8(0,0x37);
        if (!(bool)uVar5) {
          FUN_06028838();
          *(undefined2 *)(extraout_r1 + 0x2e) = 1;
          goto LAB_06028d50;
        }
        FUN_060287b8(DAT_0602975c,0x69);
      }
      uVar6 = (*(uint *)(DAT_06029760 + 0xc) & 0x80000000) == 0;
      uVar5 = 1;
    } while ((bool)uVar6);
    uVar7 = FUN_06028838();
    iVar4 = (int)((ulonglong)uVar7 >> 0x20);
    *(int *)(iVar4 + 0x20) = (int)uVar7;
    *(ushort *)(iVar4 + 0x30) = (ushort)((ulonglong)uVar7 >> 0x10) >> 0xe & 1;
    FUN_060287b8(0,0x602);
    FUN_06028cb4();
    if ((bool)uVar6) {
      FUN_060287b8(0x10000,3);
      uVar2 = 1;
    }
    else {
      FUN_060287b8(0,3);
      uVar2 = *(uint *)(DAT_06029764 + 0xc) >> 0x10;
    }
    uVar1 = FUN_06028838(uVar2);
    *(undefined2 *)(extraout_r1_00 + 0x2c) = uVar1;
    *(undefined2 *)(DAT_06029768 + 0x24) = 0x100;
    uVar1 = FUN_06028838();
    *(undefined2 *)(extraout_r1_01 + 0x3c) = uVar1;
    uVar3 = FUN_06028ca0();
    FUN_060287b8(uVar3,9);
    FUN_06028838();
    FUN_0602c1a8(DAT_0602976c,extraout_r1_02 + 0x10,0x10);
    uVar3 = FUN_06028ca0();
    FUN_060287b8(uVar3,10);
    FUN_06028838();
    FUN_0602c1a8(DAT_06029770,extraout_r1_03,0x10);
    uVar3 = FUN_06028ca0();
    FUN_060287b8(uVar3,7);
    FUN_06028cb4();
    if (!(bool)uVar6) {
      uVar3 = FUN_06028ca0();
      FUN_060287b8(uVar3,0x37);
      FUN_060287b8(0,0x6a);
    }
    FUN_060287b8(0x200,0x10);
    FUN_06028cb4();
    if ((bool)uVar6) {
      FUN_06028cc8();
      if ((bool)uVar6) {
        uVar3 = 0x400;
      }
      else {
        uVar3 = 0x100;
      }
      uVar7 = FUN_06028838(uVar3);
      iVar4 = (int)((ulonglong)uVar7 >> 0x20);
      *(int *)(iVar4 + 0x24) = (int)uVar7;
      *(undefined4 *)(iVar4 + 0x28) = 0;
    }
    else {
      FUN_06028f88(1);
      uVar3 = FUN_06028ca0();
      FUN_060287b8(uVar3,0x37);
      iVar4 = DAT_06029774;
      *(undefined2 *)(DAT_06029774 + 10) = 1;
      *(undefined4 *)(iVar4 + 0x108) = 1;
      *(undefined2 *)(iVar4 + 0x26) = 8;
      *(undefined4 *)(iVar4 + 0x104) = 8;
      FUN_060287b8(0,0x73);
      uVar7 = FUN_06028838();
      FUN_0602a03c((int)uVar7,(int)((ulonglong)uVar7 >> 0x20) + 0x24,8);
    }
    FUN_06028838();
    uVar5 = (*(uint *)(extraout_r1_04 + 0x24) & 0x400) == 0;
    if ((bool)uVar5) {
      uVar3 = 1;
    }
    else {
      uVar3 = 4;
    }
    FUN_06028f88(uVar3);
    iVar4 = DAT_06029778;
    *(undefined2 *)(DAT_06029778 + 8) = 0x100;
    *(undefined2 *)(iVar4 + 0x26) = 0x200;
    FUN_06028cb4();
    if (!(bool)uVar5) {
      uVar3 = FUN_06028ca0();
      FUN_060287b8(uVar3,0x37);
      iVar4 = DAT_0602977c;
      *(undefined2 *)(DAT_0602977c + 10) = 1;
      *(undefined4 *)(iVar4 + 0x108) = 1;
      *(undefined2 *)(iVar4 + 0x26) = 0x40;
      *(undefined4 *)(iVar4 + 0x104) = 0x40;
      uVar3 = FUN_060287b8(0,0x4d);
      FUN_0602a03c(uVar3,DAT_06029780,0x40);
    }
    uVar3 = FUN_06028ca0();
    FUN_060287b8(uVar3,0xd);
    FUN_06028838();
    *(undefined4 *)(extraout_r1_05 + 0x38) = *(undefined4 *)(DAT_06029784 + 0xc);
    FUN_06028cc8();
    if ((bool)uVar5) {
      FUN_06028838();
      software_interrupt(0x270000);
    }
  }
  return CONCAT44(param_2,param_1);
}

