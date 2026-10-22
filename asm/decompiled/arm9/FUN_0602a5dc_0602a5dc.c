// Function: FUN_0602a5dc
// Address:  0602a5dc
// Signature: undefined FUN_0602a5dc()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0602a5dc(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 uVar7;
  
  uVar5 = _DAT_0200080c;
  iVar4 = _DAT_02000808;
  iVar3 = _DAT_02000804;
  iVar2 = _DAT_02000800;
  uVar6 = _DAT_02000304;
  iVar1 = _DAT_02000300;
  _DAT_02000800 = 0;
  _DAT_02000804 = 0;
  _DAT_02000808 = 0;
  if ((iVar2 == *DAT_0602ad48 && iVar3 == DAT_0602ad48[1]) && iVar4 == DAT_0602ad48[2]) {
    _DAT_0200080c = 0;
    software_interrupt(0xe0000);
    if ((DAT_0602ad4c == uVar5 >> 0x10) &&
       (_DAT_0200080c = 0, uVar7 = DAT_0602ad54, (_DAT_02000810 & 1) != 0)) goto LAB_0602a700;
  }
  _DAT_02000300 = 0;
  if ((DAT_0602a728 == iVar1) &&
     ((_DAT_02000304 = 0, (uVar6 & 0xff00) != 0 &&
      (software_interrupt(0xe0000), DAT_0602ad58 == uVar6 >> 0x10)))) {
    uVar6 = _DAT_02000318 & 0xe;
    uVar7 = DAT_0602ad5c;
    if (uVar6 != 4) {
      if (_DAT_02000310 == 0 && _DAT_02000314 == 0) goto LAB_0602a6ec;
      uVar7 = DAT_0602ad60;
      if (uVar6 != 2) {
        if (uVar6 != 6) goto LAB_0602a6ec;
        FUN_06029e10(_DAT_02000314,DAT_0602ad64);
        FUN_06029e10(_DAT_02000310,DAT_0602ad68);
        uVar7 = FUN_0602a58c(DAT_0602ad6c);
      }
    }
    FUN_0602a4e0(uVar7,DAT_0602ad70);
    uVar7 = DAT_0602ad74;
  }
  else {
LAB_0602a6ec:
    uVar7 = 0;
  }
LAB_0602a700:
  uVar6 = FUN_06026fc0(uVar7,0x70);
  if ((uVar6 & 1) == 0) {
    uVar7 = 0;
  }
  return uVar7;
}

