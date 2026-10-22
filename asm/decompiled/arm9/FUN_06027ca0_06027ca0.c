// Function: FUN_06027ca0
// Address:  06027ca0
// Signature: undefined FUN_06027ca0()


/* WARNING: Removing unreachable block (ram,0x06028180) */
/* WARNING: Removing unreachable block (ram,0x0602818c) */
/* WARNING: Removing unreachable block (ram,0x06028194) */
/* WARNING: Removing unreachable block (ram,0x060280b8) */
/* WARNING: Removing unreachable block (ram,0x060280e0) */
/* WARNING: Removing unreachable block (ram,0x06028108) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_06027ca0(undefined4 param_1,uint param_2)

{
  undefined4 *puVar1;
  ushort *puVar2;
  int iVar3;
  ushort uVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  
  puVar1 = DAT_0602835c;
  *DAT_0602835c = param_1;
  puVar2 = DAT_060283c8;
  uVar8 = 4;
  uVar7 = param_2 << 9 | param_2 & 0x78000000 | 0x10000000;
  *(uint *)(DAT_060283c8 + 2) = uVar7 | 0x84000004;
  *(uint *)(puVar2 + 0xe) = *(uint *)(puVar2 + 0xe) & 0x7c80fffa;
  puVar2[4] = puVar2[4] & 0xfffe;
  if ((uVar7 & 0x8000000) == 0) {
    uVar5 = 4;
    iVar6 = 1;
  }
  else {
    uVar5 = 0x80;
    iVar6 = 4;
    uVar8 = 0x200;
  }
  puVar2[0x13] = (ushort)uVar5;
  puVar2[5] = (ushort)iVar6;
  if ((uVar7 & 0x8000000) == 0) {
    puVar2[0x6c] = 0;
    *(undefined4 *)(puVar2 + 0x80) = DAT_060283dc;
  }
  else {
    *(undefined4 *)(puVar2 + 0x82) = uVar5;
    *(int *)(puVar2 + 0x84) = iVar6;
    puVar2[0x6c] = 2;
    *(undefined4 *)(puVar2 + 0x80) = DAT_060283cc;
    iVar3 = DAT_060283d0;
    *(undefined4 **)(DAT_060283d0 + 4) = puVar1;
    *(ushort **)(iVar3 + 8) = puVar2 + 0x86;
    *(uint *)(iVar3 + 0xc) = uVar8 >> 2;
    *(undefined4 *)(iVar3 + 0x10) = 0x20;
    *(undefined4 *)(iVar3 + 0x14) = 0;
    *(undefined4 *)(iVar3 + 0x1c) = DAT_060283d8;
  }
  uVar4 = (ushort)DAT_060283e0;
  if (((uVar7 & 0x8000000) != 0) && (iVar6 != 1)) {
    uVar4 = uVar4 | 0x2000;
  }
  *puVar2 = uVar4;
  do {
    if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((*(uint *)(puVar2 + 0xe) & 1) == 0);
  if (((puVar2[0x6c] & 2) == 0) || ((*(uint *)(puVar2 + 0x80) & 2) == 0)) {
    do {
      if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(puVar2 + 0xe) & 0x2000000) == 0);
    _DAT_040000dc = uVar8 + 1 >> 1 | 0x80400000;
    _DAT_040000d4 = puVar1;
    _DAT_040000d8 = puVar2 + 0x18;
  }
  else {
    do {
      if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_060283e8 + 0x1c) & 0x80000000) != 0);
  }
  do {
    uVar8 = *(uint *)(puVar2 + 0xe);
    if ((uVar8 & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((uVar8 & 4) == 0);
  return uVar8;
}

