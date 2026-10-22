// Function: FUN_0001c7d0
// Address: 0001c7d0


/* WARNING: Removing unreachable block (ram,0x0001c874) */
/* WARNING: Removing unreachable block (ram,0x0001c87c) */
/* WARNING: Removing unreachable block (ram,0x0001c884) */
/* WARNING: Removing unreachable block (ram,0x0001c888) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0001c7d0(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  bool bVar15;
  
  if (param_3 == 0) {
    return;
  }
  puVar1 = param_1;
  puVar3 = param_2;
  if (((uint)param_1 & 1) != 0) {
    puVar1 = (undefined4 *)((int)param_1 + 1);
    puVar3 = (undefined4 *)((int)param_2 + 1);
    *(undefined1 *)param_2 = *(undefined1 *)param_1;
    param_3 = param_3 - 1;
    if (param_3 == 0) {
      return;
    }
  }
  if ((((uint)puVar3 & 1) == 0) && (1 < param_3)) {
    puVar2 = puVar1;
    puVar4 = puVar3;
    if (((uint)puVar1 & 2) != 0) {
      puVar2 = (undefined4 *)((int)puVar1 + 2);
      puVar4 = (undefined4 *)((int)puVar3 + 2);
      *(undefined2 *)puVar3 = *(undefined2 *)puVar1;
      param_3 = param_3 - 2;
      if (param_3 == 0) {
        return;
      }
    }
    if (((uint)puVar4 & 2) == 0) {
      if (0x27 < param_3) {
        while (0x27 < param_3) {
          uVar5 = *puVar2;
          uVar6 = puVar2[1];
          uVar7 = puVar2[2];
          uVar8 = puVar2[3];
          uVar9 = puVar2[4];
          uVar10 = puVar2[5];
          uVar11 = puVar2[6];
          uVar12 = puVar2[7];
          uVar13 = puVar2[8];
          uVar14 = puVar2[9];
          puVar2 = puVar2 + 10;
          *puVar4 = uVar5;
          puVar4[1] = uVar6;
          puVar4[2] = uVar7;
          puVar4[3] = uVar8;
          puVar4[4] = uVar9;
          puVar4[5] = uVar10;
          puVar4[6] = uVar11;
          puVar4[7] = uVar12;
          puVar4[8] = uVar13;
          puVar4[9] = uVar14;
          puVar4 = puVar4 + 10;
          param_3 = param_3 - 0x28;
        }
      }
      while (3 < param_3) {
        *puVar4 = *puVar2;
        puVar2 = puVar2 + 1;
        puVar4 = puVar4 + 1;
        param_3 = param_3 - 4;
      }
    }
    _DAT_040000ec = 0x55;
    puVar1 = puVar2;
    puVar3 = puVar4;
    while (1 < param_3) {
      *(undefined2 *)puVar3 = *(undefined2 *)puVar1;
      puVar1 = (undefined4 *)((int)puVar1 + 2);
      puVar3 = (undefined4 *)((int)puVar3 + 2);
      param_3 = param_3 - 2;
    }
  }
  while (bVar15 = param_3 != 0, param_3 = param_3 - 1, bVar15) {
    *(undefined1 *)puVar3 = *(undefined1 *)puVar1;
    puVar1 = (undefined4 *)((int)puVar1 + 1);
    puVar3 = (undefined4 *)((int)puVar3 + 1);
  }
  return;
}

