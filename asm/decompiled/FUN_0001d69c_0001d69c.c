// Function: FUN_0001d69c
// Address: 0001d69c


/* WARNING: Removing unreachable block (ram,0x0001dac0) */
/* WARNING: Removing unreachable block (ram,0x0001dacc) */
/* WARNING: Removing unreachable block (ram,0x0001dad4) */
/* WARNING: Removing unreachable block (ram,0x0001dae4) */
/* WARNING: Removing unreachable block (ram,0x0001d9f8) */
/* WARNING: Removing unreachable block (ram,0x0001da20) */
/* WARNING: Removing unreachable block (ram,0x0001da48) */

uint FUN_0001d69c(undefined4 param_1)

{
  ushort *puVar1;
  int iVar2;
  ushort uVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  ushort *puVar7;
  uint uVar8;
  bool bVar9;
  
  puVar7 = DAT_0001dc50;
  uVar6 = DAT_0001dc4c;
  *(undefined4 *)DAT_0001dc50 = param_1;
  puVar1 = DAT_0001dca0;
  uVar8 = 4;
  uVar6 = uVar6 << 9 | uVar6 & 0x78000000 | 0x10000000;
  *(uint *)(DAT_0001dca0 + 2) = uVar6 | 0x84000004;
  *(uint *)(puVar1 + 0xe) = *(uint *)(puVar1 + 0xe) & 0x7c80fffa;
  puVar1[4] = puVar1[4] & 0xfffe;
  if ((uVar6 & 0x8000000) == 0) {
    uVar4 = 4;
    iVar5 = 1;
  }
  else {
    uVar4 = 0x80;
    iVar5 = 4;
    uVar8 = 0x200;
  }
  puVar1[0x13] = (ushort)uVar4;
  puVar1[5] = (ushort)iVar5;
  if ((uVar6 & 0x8000000) == 0) {
    puVar1[0x6c] = 0;
    *(undefined4 *)(puVar1 + 0x80) = DAT_0001dcb4;
  }
  else {
    *(undefined4 *)(puVar1 + 0x82) = uVar4;
    *(int *)(puVar1 + 0x84) = iVar5;
    puVar1[0x6c] = 2;
    *(undefined4 *)(puVar1 + 0x80) = DAT_0001dca4;
    iVar2 = DAT_0001dca8;
    *(ushort **)(DAT_0001dca8 + 4) = puVar7;
    *(ushort **)(iVar2 + 8) = puVar1 + 0x86;
    *(uint *)(iVar2 + 0xc) = uVar8 >> 2;
    *(undefined4 *)(iVar2 + 0x10) = 0x20;
    *(undefined4 *)(iVar2 + 0x14) = 0;
    *(undefined4 *)(iVar2 + 0x1c) = DAT_0001dcb0;
  }
  uVar3 = (ushort)DAT_0001dcb8;
  if (((uVar6 & 0x8000000) != 0) && (iVar5 != 1)) {
    uVar3 = uVar3 | 0x2000;
  }
  *puVar1 = uVar3;
  do {
    if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((*(uint *)(puVar1 + 0xe) & 1) == 0);
  if (((puVar1[0x6c] & 2) == 0) || ((*(uint *)(puVar1 + 0x80) & 2) == 0)) {
    do {
      if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(puVar1 + 0xe) & 0x2000000) == 0);
    do {
      puVar1[0x18] = *puVar7;
      bVar9 = 1 < uVar8;
      uVar8 = uVar8 - 2;
      puVar7 = puVar7 + 1;
    } while (bVar9 && uVar8 != 0);
  }
  else {
    do {
      if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_0001dcc0 + 0x1c) & 0x80000000) != 0);
  }
  do {
    uVar8 = *(uint *)(puVar1 + 0xe);
    if ((uVar8 & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((uVar8 & 4) == 0);
  return uVar8;
}

