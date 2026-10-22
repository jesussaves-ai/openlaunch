// Function: FUN_0002091c
// Address: 0002091c


undefined8 FUN_0002091c(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  undefined4 uVar10;
  
  uVar9 = DAT_00020ae0;
  iVar5 = DAT_00020adc;
  iVar4 = DAT_00020ad8;
  iVar3 = DAT_00020ad4;
  iVar2 = DAT_00020ad0;
  iVar7 = DAT_00020acc;
  iVar6 = 0;
  uVar8 = 1;
  do {
    *(char *)(iVar2 + uVar8) = (char)iVar6;
    *(char *)(iVar7 + iVar6) = (char)uVar8;
    uVar8 = uVar8 ^ uVar8 << 1;
    if ((uVar8 & 0x100) != 0) {
      uVar8 = uVar8 ^ uVar9;
    }
    iVar6 = iVar6 + 1;
  } while (iVar6 != 0x100);
  iVar6 = 0;
  uVar9 = 99;
  do {
    *(char *)(iVar4 + uVar9) = (char)iVar6;
    *(char *)(iVar3 + iVar6) = (char)uVar9;
    iVar6 = iVar6 + 1;
    bVar1 = *(byte *)(iVar7 + (0xff - (uint)*(byte *)(iVar2 + iVar6)));
    uVar9 = (uint)bVar1;
    uVar9 = (uVar9 ^ 99 ^ ((uint)(bVar1 >> 7) | uVar9 << 1) ^ ((uint)(bVar1 >> 6) | uVar9 << 2) ^
             ((uint)(bVar1 >> 5) | uVar9 << 3) ^ ((uint)(bVar1 >> 4) | uVar9 << 4)) & 0xff;
  } while (iVar6 != 0x100);
  iVar7 = 0;
  do {
    uVar10 = 0;
    uVar9 = (uint)*(byte *)(iVar4 + iVar7);
    if (uVar9 != 0) {
      FUN_000209f0(iVar7,*(undefined1 *)(iVar2 + uVar9),0,*(undefined1 *)(iVar2 + 0xb),param_1,
                   param_2,param_3,param_4);
      FUN_000209f0();
      FUN_000209f0();
      iVar7 = FUN_000209f0();
    }
    *(undefined4 *)(iVar5 + iVar7 * 4) = uVar10;
    iVar7 = iVar7 + 1;
  } while (iVar7 != 0x100);
  return CONCAT44(param_2,param_1);
}

