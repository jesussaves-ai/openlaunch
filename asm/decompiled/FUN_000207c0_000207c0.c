// Function: FUN_000207c0
// Address: 000207c0


undefined8 FUN_000207c0(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  undefined8 uVar10;
  
  uVar10 = FUN_0002091c();
  uVar5 = (uint)((ulonglong)uVar10 >> 0x20);
  uVar2 = uVar5 >> 3;
  bVar9 = uVar2 == 0x10 || uVar2 == 0x18;
  if (uVar2 != 0x10 && uVar2 != 0x18) {
    bVar9 = uVar2 == 0x20;
  }
  do {
  } while (!bVar9);
  uVar5 = uVar5 >> 5;
  *DAT_00020aac = uVar5 + 6;
  iVar4 = DAT_00020ab0;
  uVar5 = uVar5 + 7;
  iVar1 = uVar5 * 0x10;
  iVar6 = 0;
  uVar7 = 0;
  uVar8 = 0x1000000;
  do {
    if ((int)(iVar6 - uVar2) < 0) {
      uVar5 = *(uint *)((int)uVar10 + iVar6);
    }
    else {
      uVar5 = uVar5 ^ *(uint *)(iVar4 + (iVar6 - uVar2));
    }
    *(uint *)(iVar4 + iVar6) = uVar5;
    uVar7 = uVar7 + 4;
    if (uVar7 == uVar2) {
      uVar7 = 0;
      uVar5 = FUN_00020890();
      uVar5 = (uVar5 >> 8 | uVar5 << 0x18) ^ uVar8 >> 0x18;
      uVar3 = uVar8 >> 0x1f;
      uVar8 = uVar8 << 1;
      if (SUB41(uVar3,0)) {
        uVar8 = uVar8 ^ 0x1b000000;
      }
    }
    if (uVar7 == 0x10 && uVar2 == 0x20) {
      uVar5 = FUN_00020890();
    }
    iVar6 = iVar6 + 4;
  } while (iVar6 != iVar1);
  return CONCAT44(param_2,param_1);
}

