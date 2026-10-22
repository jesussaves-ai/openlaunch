// Function: FUN_0001f068
// Address: 0001f068


/* WARNING: Removing unreachable block (ram,0x0001f120) */
/* WARNING: Removing unreachable block (ram,0x0001f11c) */
/* WARNING: Removing unreachable block (ram,0x0001f124) */

void FUN_0001f068(void)

{
  byte bVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  undefined2 uVar5;
  undefined2 uVar6;
  undefined2 uVar7;
  undefined2 *puVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  
  puVar8 = DAT_0001f85c;
  uVar10 = *(ushort *)(DAT_0001f858 + 2) - 0x10;
  if (*(char *)(DAT_0001f858 + 1) == '\x02') {
    uVar10 = uVar10 - *(byte *)(DAT_0001f858 + 4);
  }
  bVar1 = *(byte *)(DAT_0001f858 + 6);
  uVar2 = *(undefined2 *)(DAT_0001f858 + 8);
  uVar3 = *(undefined2 *)(DAT_0001f858 + 10);
  uVar4 = *(undefined2 *)(DAT_0001f858 + 0xc);
  uVar5 = *(undefined2 *)(DAT_0001f858 + 0xe);
  uVar6 = *(undefined2 *)(DAT_0001f858 + 0x10);
  uVar7 = *(undefined2 *)(DAT_0001f858 + 0x12);
  if ((CONCAT22(*(undefined2 *)(DAT_0001f858 + 0x14),*(undefined2 *)(DAT_0001f858 + 0x14)) &
      0xffffff) >> 8 == uVar10) {
    *DAT_0001f85c = 0x18;
    puVar8[1] = 0x40;
    puVar8[2] = 0;
    puVar8[3] = 10;
    puVar8[4] = (short)uVar10 + 0x18;
    puVar8[5] = (ushort)bVar1;
    puVar8[6] = (ushort)bVar1;
    puVar8[6] = 8;
    puVar8[7] = 0;
    puVar8[8] = uVar2;
    puVar8[9] = uVar3;
    puVar8[10] = uVar4;
    puVar8[0xb] = uVar5;
    puVar8[0xc] = uVar6;
    puVar8[0xd] = uVar7;
    puVar8[0x11] = 0;
    uVar9 = *DAT_0001f864;
    bVar11 = (uVar9 & 1) == 0;
    if (bVar11) {
      uVar9 = (uint)*DAT_0001f868;
    }
    if (!bVar11 || (uVar9 & 0x10) != 0) {
      *DAT_0001f86c = 0;
      FUN_000265c8(DAT_0001f870,uVar10 + 0x27 & 0xfffffffc,0xffffffff,0);
      return;
    }
  }
  FUN_000296b4(0x3f);
  return;
}

