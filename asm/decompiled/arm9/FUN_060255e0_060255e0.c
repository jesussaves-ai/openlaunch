// Function: FUN_060255e0
// Address:  060255e0
// Signature: undefined FUN_060255e0()


void FUN_060255e0(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  uint uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  undefined4 uVar15;
  bool bVar16;
  
  uVar5 = DAT_060256fc;
  if (DAT_060256fc == 0) {
    return;
  }
  puVar1 = DAT_060256f4;
  puVar3 = DAT_060256f8;
  if (((uint)DAT_060256f4 & 1) != 0) {
    puVar1 = (undefined4 *)((int)DAT_060256f4 + 1);
    puVar3 = (undefined4 *)((int)DAT_060256f8 + 1);
    *(undefined1 *)DAT_060256f8 = *(undefined1 *)DAT_060256f4;
    uVar5 = uVar5 - 1;
    if (uVar5 == 0) {
      return;
    }
  }
  puVar2 = puVar1;
  puVar4 = puVar3;
  if ((((uint)puVar3 & 1) == 0) && (1 < uVar5)) {
    if (((uint)puVar1 & 2) != 0) {
      puVar2 = (undefined4 *)((int)puVar1 + 2);
      puVar4 = (undefined4 *)((int)puVar3 + 2);
      *(undefined2 *)puVar3 = *(undefined2 *)puVar1;
      uVar5 = uVar5 - 2;
      if (uVar5 == 0) {
        return;
      }
    }
    if (((uint)puVar4 & 2) == 0) {
      if (0x27 < uVar5) {
        while (0x27 < uVar5) {
          uVar6 = *puVar2;
          uVar7 = puVar2[1];
          uVar8 = puVar2[2];
          uVar9 = puVar2[3];
          uVar10 = puVar2[4];
          uVar11 = puVar2[5];
          uVar12 = puVar2[6];
          uVar13 = puVar2[7];
          uVar14 = puVar2[8];
          uVar15 = puVar2[9];
          puVar2 = puVar2 + 10;
          *puVar4 = uVar6;
          puVar4[1] = uVar7;
          puVar4[2] = uVar8;
          puVar4[3] = uVar9;
          puVar4[4] = uVar10;
          puVar4[5] = uVar11;
          puVar4[6] = uVar12;
          puVar4[7] = uVar13;
          puVar4[8] = uVar14;
          puVar4[9] = uVar15;
          puVar4 = puVar4 + 10;
          uVar5 = uVar5 - 0x28;
        }
      }
      while (3 < uVar5) {
        *puVar4 = *puVar2;
        puVar2 = puVar2 + 1;
        puVar4 = puVar4 + 1;
        uVar5 = uVar5 - 4;
      }
    }
    while (1 < uVar5) {
      *(undefined2 *)puVar4 = *(undefined2 *)puVar2;
      puVar2 = (undefined4 *)((int)puVar2 + 2);
      puVar4 = (undefined4 *)((int)puVar4 + 2);
      uVar5 = uVar5 - 2;
    }
  }
  while (bVar16 = uVar5 != 0, uVar5 = uVar5 - 1, bVar16) {
    *(undefined1 *)puVar4 = *(undefined1 *)puVar2;
    puVar2 = (undefined4 *)((int)puVar2 + 1);
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  return;
}

