// Function: FUN_000269c0
// Address: 000269c0


void FUN_000269c0(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  ushort *puVar4;
  uint uVar5;
  undefined2 *puVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  byte *pbVar12;
  byte *pbVar13;
  
  iVar3 = DAT_00026e18;
  iVar2 = DAT_00026e14;
  iVar8 = 0x10;
  puVar4 = DAT_00026e1c;
  puVar6 = (undefined2 *)(DAT_00026e14 + 0x44);
  do {
    *(undefined2 *)(iVar3 + (uint)*puVar4) = *puVar6;
    iVar8 = iVar8 + -1;
    puVar4 = puVar4 + 1;
    puVar6 = puVar6 + 1;
  } while (iVar8 != 0);
  uVar10 = (uint)*(byte *)(iVar2 + 0x41) + (*(byte *)(iVar2 + 0x41) & 0x80);
  *(short *)(DAT_00026e20 + 0x84) = (short)uVar10;
  uVar11 = (uint)*(byte *)(iVar2 + 0x42);
  pbVar12 = (byte *)(iVar2 + 0xce);
  cVar1 = *(char *)(iVar2 + 0x40);
  if (cVar1 == '\x03') {
    uVar10 = 0;
    do {
      FUN_00026928((uint)*pbVar12 | uVar10 << 8 | 0x50000);
      uVar10 = uVar10 + 1;
      pbVar12 = pbVar12 + 1;
    } while (uVar10 != uVar11);
  }
  else {
    do {
      uVar5 = 0;
      uVar9 = 0;
      pbVar13 = pbVar12;
      do {
        pbVar12 = pbVar13 + 1;
        uVar7 = (uint)*pbVar13;
        uVar5 = uVar5 | uVar7 << (uVar9 & 0xff);
        uVar9 = uVar9 + 8;
        pbVar13 = pbVar12;
      } while (uVar9 < (uVar10 & 0x3f));
      if (cVar1 == '\x02') {
        uVar7 = uVar5 >> 0x12;
      }
      if (cVar1 == '\x02' && uVar7 == 9) {
        *DAT_00026e24 = uVar5 & 0xffff83ff;
      }
      FUN_00026928();
      uVar11 = uVar11 - 1;
    } while (uVar11 != 0);
  }
  return;
}

