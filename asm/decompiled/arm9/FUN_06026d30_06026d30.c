// Function: FUN_06026d30
// Address:  06026d30
// Signature: undefined FUN_06026d30()


undefined8 FUN_06026d30(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  int iVar5;
  int unaff_r11;
  undefined1 in_ZR;
  undefined1 uVar6;
  undefined8 uVar7;
  
  FUN_06026df0(param_1,param_2,DAT_06027480,DAT_06027484,param_1,param_2,param_3,param_4);
  if ((((!(bool)in_ZR) && (*(int *)(unaff_r11 + 0x28) + 0xfe000000U < DAT_06027488)) &&
      (uVar4 = *(uint *)(unaff_r11 + 0x20) & 0xfffff000, 0x3fff < uVar4)) &&
     (uVar6 = uVar4 == 0x8000, uVar4 < 0x8000)) {
    FUN_06026de8();
    if ((!(bool)uVar6) && (FUN_06026e08(), !(bool)uVar6)) {
      uVar1 = FUN_06026c50(*(undefined4 *)(unaff_r11 + 0xc),8,2);
      FUN_06026b48(uVar1,*(undefined4 *)(unaff_r11 + 0x28));
      uVar1 = FUN_06026c50(*(undefined4 *)(unaff_r11 + 0xc),8,3);
      iVar2 = *(int *)(unaff_r11 + 0x28);
      iVar5 = 0x800;
      do {
        uVar7 = FUN_06026b48(uVar1,iVar2);
        uVar1 = (undefined4)uVar7;
        iVar2 = (int)((ulonglong)uVar7 >> 0x20) + 8;
        iVar5 = iVar5 + -8;
        uVar6 = iVar5 == 0;
      } while (!(bool)uVar6);
    }
    FUN_06026de8();
    uVar1 = DAT_0602748c;
    if ((bool)uVar6) {
      iVar2 = 8;
    }
    else {
      iVar2 = 0x800;
    }
    puVar3 = *(undefined4 **)(unaff_r11 + 0x28);
    do {
      *puVar3 = uVar1;
      iVar2 = iVar2 + -4;
      puVar3 = puVar3 + 1;
    } while (iVar2 != 0);
  }
  return CONCAT44(param_2,param_1);
}

