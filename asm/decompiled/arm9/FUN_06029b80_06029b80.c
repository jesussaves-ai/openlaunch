// Function: FUN_06029b80
// Address:  06029b80
// Signature: undefined FUN_06029b80()


void FUN_06029b80(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  uint uVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  int unaff_r7;
  undefined4 uVar12;
  int unaff_r8;
  undefined4 uVar13;
  int unaff_r9;
  undefined4 uVar14;
  undefined4 uVar15;
  int *unaff_r11;
  undefined4 uVar16;
  undefined4 uVar17;
  bool bVar18;
  
  iVar1 = unaff_r11[1];
  puVar4 = (undefined4 *)unaff_r11[3];
  uVar6 = unaff_r11[2];
  if (*unaff_r11 != unaff_r9) {
    return;
  }
  if (uVar6 == 0) {
    return;
  }
  if (unaff_r8 != unaff_r7) {
    if (unaff_r7 == 7) {
      FUN_060270f8();
    }
    if (unaff_r7 == 9) {
      FUN_06027128();
    }
  }
  uVar7 = *DAT_0602a2e4 - iVar1;
  if (uVar7 < uVar6) {
    uVar6 = uVar7;
  }
  puVar2 = (undefined4 *)(iVar1 + DAT_0602a2e8);
  unaff_r11[2] = unaff_r11[2] - uVar6;
  unaff_r11[3] = unaff_r11[3] + uVar6;
  *unaff_r11 = *unaff_r11 + 1;
  unaff_r11[1] = 0;
  if (uVar6 == 0) {
    return;
  }
  puVar3 = puVar2;
  puVar5 = puVar4;
  if (((uint)puVar2 & 1) != 0) {
    puVar3 = (undefined4 *)((int)puVar2 + 1);
    puVar5 = (undefined4 *)((int)puVar4 + 1);
    *(undefined1 *)puVar4 = *(undefined1 *)puVar2;
    uVar6 = uVar6 - 1;
    if (uVar6 == 0) {
      return;
    }
  }
  puVar4 = puVar3;
  puVar2 = puVar5;
  if ((((uint)puVar5 & 1) == 0) && (1 < uVar6)) {
    if (((uint)puVar3 & 2) != 0) {
      puVar4 = (undefined4 *)((int)puVar3 + 2);
      puVar2 = (undefined4 *)((int)puVar5 + 2);
      *(undefined2 *)puVar5 = *(undefined2 *)puVar3;
      uVar6 = uVar6 - 2;
      if (uVar6 == 0) {
        return;
      }
    }
    if (((uint)puVar2 & 2) == 0) {
      if (0x27 < uVar6) {
        while (0x27 < uVar6) {
          uVar8 = *puVar4;
          uVar9 = puVar4[1];
          uVar10 = puVar4[2];
          uVar11 = puVar4[3];
          uVar12 = puVar4[4];
          uVar13 = puVar4[5];
          uVar14 = puVar4[6];
          uVar15 = puVar4[7];
          uVar16 = puVar4[8];
          uVar17 = puVar4[9];
          puVar4 = puVar4 + 10;
          *puVar2 = uVar8;
          puVar2[1] = uVar9;
          puVar2[2] = uVar10;
          puVar2[3] = uVar11;
          puVar2[4] = uVar12;
          puVar2[5] = uVar13;
          puVar2[6] = uVar14;
          puVar2[7] = uVar15;
          puVar2[8] = uVar16;
          puVar2[9] = uVar17;
          puVar2 = puVar2 + 10;
          uVar6 = uVar6 - 0x28;
        }
      }
      while (3 < uVar6) {
        *puVar2 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar2 = puVar2 + 1;
        uVar6 = uVar6 - 4;
      }
    }
    while (1 < uVar6) {
      *(undefined2 *)puVar2 = *(undefined2 *)puVar4;
      puVar4 = (undefined4 *)((int)puVar4 + 2);
      puVar2 = (undefined4 *)((int)puVar2 + 2);
      uVar6 = uVar6 - 2;
    }
  }
  while (bVar18 = uVar6 != 0, uVar6 = uVar6 - 1, bVar18) {
    *(undefined1 *)puVar2 = *(undefined1 *)puVar4;
    puVar4 = (undefined4 *)((int)puVar4 + 1);
    puVar2 = (undefined4 *)((int)puVar2 + 1);
  }
  return;
}

