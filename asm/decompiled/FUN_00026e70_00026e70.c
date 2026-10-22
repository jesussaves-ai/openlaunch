// Function: FUN_00026e70
// Address: 00026e70


void FUN_00026e70(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  uint uVar8;
  bool bVar9;
  
  uVar3 = FUN_00020ae4();
  iVar2 = DAT_00027890;
  iVar7 = 6;
  do {
    if ((uint)*(ushort *)(iVar2 + 0x54) == (uint)*(ushort *)(iVar2 + 0x5a)) break;
    iVar1 = (uint)*(ushort *)(iVar2 + 0x5a) * 2;
    iVar4 = FUN_00026bc8(iVar1,8);
    uVar8 = iVar4 + 0xfU & 0xfffffffc;
    uVar5 = FUN_00027dac(iVar1,uVar8);
    uVar6 = *DAT_00027898;
    bVar9 = (uVar6 & uVar5) == 0;
    if (bVar9) {
      uVar6 = (uint)*DAT_0002789c;
    }
    if (!bVar9 || (uVar6 & 0x10) != 0) {
      *DAT_000278a0 = 0;
      FUN_000265c8(iVar1 + DAT_000278a4,uVar8,DAT_000278a4 + 8000,&DAT_00001340);
    }
    uVar8 = iVar1 + uVar8;
    if (7999 < uVar8) {
      uVar8 = uVar8 - 0x1340;
    }
    *(short *)(iVar2 + 0x5a) = (short)(uVar8 >> 1);
    iVar7 = iVar7 + -1;
  } while (iVar7 != 0);
  FUN_00020af8(uVar3);
  return;
}

