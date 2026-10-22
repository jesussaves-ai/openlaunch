// Function: FUN_000208b4
// Address: 000208b4


undefined8 FUN_000208b4(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  int iVar10;
  int iVar11;
  
  FUN_000207c0();
  iVar10 = *DAT_00020ab8;
  iVar11 = iVar10 * 4;
  puVar9 = DAT_00020abc + iVar10 * 4;
  puVar8 = DAT_00020abc;
  do {
    uVar1 = *puVar8;
    uVar2 = puVar8[1];
    uVar3 = puVar8[2];
    uVar4 = puVar8[3];
    uVar5 = puVar9[1];
    uVar6 = puVar9[2];
    uVar7 = puVar9[3];
    *puVar8 = *puVar9;
    puVar8[1] = uVar5;
    puVar8[2] = uVar6;
    puVar8[3] = uVar7;
    *puVar9 = uVar1;
    puVar9[1] = uVar2;
    puVar9[2] = uVar3;
    puVar9[3] = uVar4;
    puVar8 = puVar8 + 4;
    puVar9 = puVar9 + -4;
    iVar10 = iVar10 + -2;
  } while (iVar10 != 0);
  iVar11 = iVar11 + -4;
  puVar8 = DAT_00020ac8;
  do {
    FUN_00020890(*puVar8);
    uVar1 = FUN_00020860();
    *puVar8 = uVar1;
    iVar11 = iVar11 + -1;
    puVar8 = puVar8 + 1;
  } while (iVar11 != 0);
  return CONCAT44(param_2,param_1);
}

