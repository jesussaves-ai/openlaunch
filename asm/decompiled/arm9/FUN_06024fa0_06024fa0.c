// Function: FUN_06024fa0
// Address:  06024fa0
// Signature: undefined FUN_06024fa0()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_06024fa0(ushort *param_1,ushort *param_2,int param_3)

{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  ushort *puVar5;
  uint uVar6;
  ushort *puVar7;
  ushort *puVar8;
  ushort *puVar9;
  ushort *puVar10;
  ushort *puVar11;
  int iVar12;
  bool bVar13;
  
  puVar9 = param_1;
  puVar11 = param_2;
LAB_06024fb0:
  while( true ) {
    uVar1 = *puVar9;
    puVar8 = puVar9 + 3;
    uVar6 = (uint)puVar9[2];
    if (uVar1 == 0) {
      return CONCAT44(param_2,param_1);
    }
    uVar3 = uVar1 & 0xffff7fff;
    puVar5 = puVar11;
    iVar12 = param_3;
    if (((uVar1 & 0x8000) == 0) || ((*DAT_060251d4 & 3) == 0)) break;
LAB_0602507c:
    puVar9 = (ushort *)((int)puVar8 + uVar6 + uVar3);
  }
LAB_06024fe8:
  while( true ) {
    puVar11 = puVar5;
    puVar5 = puVar11 + 1;
    iVar12 = iVar12 + -2;
    if (iVar12 == 0) break;
    uVar4 = uVar3;
    puVar7 = puVar8;
    puVar10 = puVar11;
    if (*puVar11 == *puVar8) goto LAB_06025008;
  }
  _DAT_04000000 = 0x10000;
  iVar12 = 0x10000;
  do {
    _DAT_05000000 = iVar12;
    iVar12 = _DAT_05000000 + 0x7b;
  } while (DAT_04000130 == -1);
  puVar11 = (ushort *)((int)puVar5 - param_3);
  goto LAB_0602507c;
LAB_06025008:
  do {
    uVar2 = (uint)*puVar7;
    bVar13 = uVar2 == DAT_060251d8 || uVar2 == *puVar10;
    if (uVar2 != DAT_060251d8 && uVar2 != *puVar10) break;
    bVar13 = uVar4 - 2 == 0;
    uVar4 = uVar4 - 2;
    puVar7 = puVar7 + 1;
    puVar10 = puVar10 + 1;
  } while (!bVar13);
  if (bVar13) goto code_r0x0602502c;
  goto LAB_06024fe8;
code_r0x0602502c:
  puVar5 = (ushort *)((int)(short)puVar9[1] + (int)puVar11);
  puVar8 = (ushort *)((int)puVar8 + uVar3);
  do {
    puVar9 = puVar8 + 1;
    *puVar5 = *puVar8;
    uVar6 = uVar6 - 2;
    puVar5 = puVar5 + 1;
    puVar8 = puVar9;
    param_3 = iVar12;
  } while (uVar6 != 0);
  goto LAB_06024fb0;
}

