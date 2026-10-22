// Function: FUN_06027d50
// Address:  06027d50
// Signature: undefined FUN_06027d50()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_06027d50(undefined4 param_1,undefined4 param_2,uint param_3)

{
  ushort *puVar1;
  ushort *puVar2;
  int iVar3;
  ushort uVar4;
  uint uVar5;
  undefined4 uVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  
  puVar2 = DAT_060283c8;
  puVar1 = DAT_0602836c;
  uVar7 = (DAT_06028370 - param_3) * 0x200 | DAT_06028370 - param_3 & 0x78000000 | 0x4000000 |
          param_3 & 0xfffffdff;
  *(uint *)(DAT_060283c8 + 2) = uVar7;
  *(uint *)(puVar2 + 0xe) = *(uint *)(puVar2 + 0xe) & 0x7c80fffa;
  puVar2[4] = puVar2[4] & 0xfffe;
  if ((uVar7 & 0x8000000) == 0) {
    uVar8 = param_3;
    param_3 = 1;
    uVar5 = param_3;
  }
  else {
    uVar5 = 0x80;
    uVar8 = param_3 << 7;
  }
  puVar2[0x13] = (ushort)uVar5;
  puVar2[5] = (ushort)param_3;
  if ((uVar7 & 0x8000000) == 0) {
    puVar2[0x6c] = 0;
    *(undefined4 *)(puVar2 + 0x80) = DAT_060283dc;
  }
  else {
    *(uint *)(puVar2 + 0x82) = uVar5;
    *(uint *)(puVar2 + 0x84) = param_3;
    puVar2[0x6c] = 2;
    *(undefined4 *)(puVar2 + 0x80) = DAT_060283cc;
    iVar3 = DAT_060283d0;
    bVar9 = (uVar7 & 0x80000000) == 0;
    if (bVar9) {
      *(ushort **)(DAT_060283d0 + 4) = puVar2 + 0x86;
      *(ushort **)(iVar3 + 8) = puVar1;
    }
    else {
      *(ushort **)(DAT_060283d0 + 4) = puVar1;
      *(ushort **)(iVar3 + 8) = puVar2 + 0x86;
    }
    *(uint *)(iVar3 + 0xc) = uVar8 >> 2;
    *(undefined4 *)(iVar3 + 0x10) = 0x20;
    *(undefined4 *)(iVar3 + 0x14) = 0;
    uVar6 = DAT_060283d4;
    if (!bVar9) {
      uVar6 = DAT_060283d8;
    }
    *(undefined4 *)(iVar3 + 0x1c) = uVar6;
  }
  if ((uVar7 & 0x80000000) == 0) {
    uVar4 = (ushort)DAT_060283e4;
  }
  else {
    uVar4 = (ushort)DAT_060283e0;
  }
  if (((uVar7 & 0x8000000) != 0) && (param_3 != 1)) {
    uVar4 = uVar4 | 0x2000;
  }
  *puVar2 = uVar4;
  do {
    if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((*(uint *)(puVar2 + 0xe) & 1) == 0);
  if (((puVar2[0x6c] & 2) == 0) || ((*(uint *)(puVar2 + 0x80) & 2) == 0)) {
    if ((uVar7 & 0x80000000) == 0) {
      do {
        if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
          return 0xffffffff;
        }
      } while ((*(uint *)(puVar2 + 0xe) & 0x1000000) == 0);
      _DAT_040000dc = uVar8 + 1 >> 1 | 0x81000000;
      _DAT_040000d4 = puVar2 + 0x18;
      _DAT_040000d8 = puVar1;
    }
    else {
      do {
        if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
          return 0xffffffff;
        }
      } while ((*(uint *)(puVar2 + 0xe) & 0x2000000) == 0);
      _DAT_040000dc = uVar8 + 1 >> 1 | 0x80400000;
      _DAT_040000d4 = puVar1;
      _DAT_040000d8 = puVar2 + 0x18;
    }
  }
  else {
    do {
      if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_060283e8 + 0x1c) & 0x80000000) != 0);
  }
  do {
    uVar7 = *(uint *)(puVar2 + 0xe);
    if ((uVar7 & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((uVar7 & 4) == 0);
  return uVar7;
}

