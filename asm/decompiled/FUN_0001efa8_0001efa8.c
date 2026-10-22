// Function: FUN_0001efa8
// Address: 0001efa8


void FUN_0001efa8(void)

{
  ushort uVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  undefined2 *puVar5;
  undefined2 uVar6;
  int iVar7;
  undefined2 unaff_r4;
  undefined2 unaff_r5;
  undefined2 unaff_r6;
  bool bVar8;
  
  if ((*DAT_0001f83c == '\0') &&
     (iVar7 = FUN_000264e4(DAT_0001f840), puVar5 = DAT_0001f848, iVar7 != 0)) {
    uVar1 = *(ushort *)(DAT_0001f844 + 10);
    bVar8 = (*(ushort *)(DAT_0001f844 + 0xc) & 0x100) == 0;
    if (bVar8) {
      unaff_r4 = *(undefined2 *)(DAT_0001f844 + 0x10);
      unaff_r5 = *(undefined2 *)(DAT_0001f844 + 0x12);
      unaff_r6 = *(undefined2 *)(DAT_0001f844 + 0x14);
    }
    uVar2 = *(undefined2 *)(DAT_0001f844 + 0x16);
    uVar3 = *(undefined2 *)(DAT_0001f844 + 0x18);
    uVar4 = *(undefined2 *)(DAT_0001f844 + 0x1a);
    if (!bVar8) {
      unaff_r4 = *(undefined2 *)(DAT_0001f844 + 0x1c);
      unaff_r5 = *(undefined2 *)(DAT_0001f844 + 0x1e);
      unaff_r6 = *(undefined2 *)(DAT_0001f844 + 0x20);
    }
    *DAT_0001f848 = (short)DAT_0001f84c;
    puVar5[1] = uVar1 - 0xc;
    puVar5[2] = 0;
    if (DAT_0001f850 == (ushort)puVar5[0xe]) {
      uVar6 = 0x1c00;
    }
    else {
      uVar6 = 0;
    }
    puVar5[3] = uVar6;
    puVar5[4] = unaff_r4;
    puVar5[5] = unaff_r5;
    puVar5[6] = unaff_r6;
    puVar5[7] = uVar2;
    puVar5[8] = uVar3;
    puVar5[9] = uVar4;
    iVar7 = uVar1 - 0x1c;
    puVar5[10] = (short)iVar7 * 0x100 | (ushort)((uint)iVar7 >> 8);
    FUN_0001f1b4(DAT_0001f854);
  }
  return;
}

