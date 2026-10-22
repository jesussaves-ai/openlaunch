// Function: FUN_06027ff0
// Address:  06027ff0
// Signature: undefined FUN_06027ff0()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_06027ff0(ushort *param_1,uint param_2,uint param_3)

{
  ushort *puVar1;
  int iVar2;
  ushort uVar3;
  uint uVar4;
  undefined4 uVar5;
  uint uVar6;
  uint uVar7;
  bool bVar8;
  
  puVar1 = DAT_060283c8;
  uVar6 = param_2 << 9 | param_2 & 0x78000000 | 0x4000000 | param_3 & 0xfffffdff;
  *(uint *)(DAT_060283c8 + 2) = uVar6;
  *(uint *)(puVar1 + 0xe) = *(uint *)(puVar1 + 0xe) & 0x7c80fffa;
  puVar1[4] = puVar1[4] & 0xfffe;
  if ((uVar6 & 0x8000000) == 0) {
    uVar7 = param_3;
    param_3 = 1;
    uVar4 = param_3;
  }
  else {
    uVar4 = 0x80;
    uVar7 = param_3 << 7;
  }
  puVar1[0x13] = (ushort)uVar4;
  puVar1[5] = (ushort)param_3;
  if ((uVar6 & 0x8000000) == 0) {
    puVar1[0x6c] = 0;
    *(undefined4 *)(puVar1 + 0x80) = DAT_060283dc;
  }
  else {
    *(uint *)(puVar1 + 0x82) = uVar4;
    *(uint *)(puVar1 + 0x84) = param_3;
    puVar1[0x6c] = 2;
    *(undefined4 *)(puVar1 + 0x80) = DAT_060283cc;
    iVar2 = DAT_060283d0;
    bVar8 = (uVar6 & 0x80000000) == 0;
    if (bVar8) {
      *(ushort **)(DAT_060283d0 + 4) = puVar1 + 0x86;
      *(ushort **)(iVar2 + 8) = param_1;
    }
    else {
      *(ushort **)(DAT_060283d0 + 4) = param_1;
      *(ushort **)(iVar2 + 8) = puVar1 + 0x86;
    }
    *(uint *)(iVar2 + 0xc) = uVar7 >> 2;
    *(undefined4 *)(iVar2 + 0x10) = 0x20;
    *(undefined4 *)(iVar2 + 0x14) = 0;
    uVar5 = DAT_060283d4;
    if (!bVar8) {
      uVar5 = DAT_060283d8;
    }
    *(undefined4 *)(iVar2 + 0x1c) = uVar5;
  }
  if ((uVar6 & 0x80000000) == 0) {
    uVar3 = (ushort)DAT_060283e4;
  }
  else {
    uVar3 = (ushort)DAT_060283e0;
  }
  if (((uVar6 & 0x8000000) != 0) && (param_3 != 1)) {
    uVar3 = uVar3 | 0x2000;
  }
  *puVar1 = uVar3;
  do {
    if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((*(uint *)(puVar1 + 0xe) & 1) == 0);
  if (((puVar1[0x6c] & 2) == 0) || ((*(uint *)(puVar1 + 0x80) & 2) == 0)) {
    if ((uVar6 & 0x80000000) == 0) {
      do {
        if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
          return 0xffffffff;
        }
      } while ((*(uint *)(puVar1 + 0xe) & 0x1000000) == 0);
      _DAT_040000dc = uVar7 + 1 >> 1 | 0x81000000;
      _DAT_040000d4 = puVar1 + 0x18;
      _DAT_040000d8 = param_1;
    }
    else {
      do {
        if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
          return 0xffffffff;
        }
      } while ((*(uint *)(puVar1 + 0xe) & 0x2000000) == 0);
      _DAT_040000dc = uVar7 + 1 >> 1 | 0x80400000;
      _DAT_040000d8 = puVar1 + 0x18;
      _DAT_040000d4 = param_1;
    }
  }
  else {
    do {
      if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_060283e8 + 0x1c) & 0x80000000) != 0);
  }
  do {
    uVar6 = *(uint *)(puVar1 + 0xe);
    if ((uVar6 & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((uVar6 & 4) == 0);
  return uVar6;
}

