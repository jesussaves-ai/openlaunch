// Function: FUN_00024728
// Address: 00024728


int FUN_00024728(int param_1,int param_2,uint param_3)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 *unaff_r4;
  int iVar4;
  undefined2 *unaff_r5;
  int iVar5;
  int unaff_r6;
  int unaff_r7;
  uint unaff_r8;
  int iVar6;
  int *piVar7;
  int *piVar8;
  bool bVar9;
  bool bVar10;
  bool bVar11;
  
  bVar9 = unaff_r4 == (undefined4 *)0x0;
  bVar10 = unaff_r5 == (undefined2 *)0x0;
  if (!bVar9 && !bVar10) {
    unaff_r6 = param_1;
  }
  bVar11 = param_1 == 0;
  if ((!bVar9 && !bVar10) && !bVar11) {
    unaff_r7 = param_2;
  }
  if (((!bVar9 && !bVar10) && !bVar11) && param_2 != 0) {
    unaff_r8 = param_3;
  }
  if ((((bVar9 || bVar10) || bVar11) || param_2 == 0) || param_3 == 0) {
    iVar5 = FUN_0001cd28();
  }
  else {
    uVar1 = FUN_00020ae4();
    piVar7 = *(int **)(unaff_r6 + 0x14);
    if (piVar7 == (int *)0x0) {
      FUN_00020af8(uVar1);
      iVar5 = FUN_0001cd68();
    }
    else if (unaff_r8 < *piVar7 - 0xcU) {
      FUN_00020af8(uVar1);
      iVar5 = FUN_0001cd70();
    }
    else {
      puVar2 = (undefined4 *)piVar7[3];
      *unaff_r4 = *puVar2;
      *unaff_r5 = *(undefined2 *)(puVar2 + 1);
      iVar4 = 0xc;
      iVar5 = 0;
      iVar6 = *piVar7;
      while ((iVar6 != 0 && (piVar7 != (int *)0x0))) {
        iVar3 = unaff_r7 + iVar5;
        iVar6 = iVar6 - piVar7[1];
        iVar5 = iVar5 + (piVar7[1] - iVar4);
        FUN_0001c7d0(piVar7[3] + iVar4,iVar3);
        piVar8 = (int *)piVar7[2];
        *(int **)(unaff_r6 + 0x14) = piVar8;
        piVar7[2] = 0;
        FUN_000223c8();
        iVar4 = 0;
        piVar7 = piVar8;
      }
      if (piVar7 == (int *)0x0) {
        *(undefined4 *)(unaff_r6 + 0x18) = 0;
      }
      FUN_00020af8(uVar1);
    }
  }
  return iVar5;
}

