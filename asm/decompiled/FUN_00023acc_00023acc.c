// Function: FUN_00023acc
// Address: 00023acc


void FUN_00023acc(void)

{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int *piVar5;
  undefined4 *puVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  bool bVar10;
  
  iVar1 = *DAT_0002481c;
  iVar4 = *DAT_00024820;
  *DAT_00024820 = iVar1;
  uVar8 = iVar1 - iVar4;
  iVar1 = *DAT_00024824;
  puVar6 = DAT_00024828;
  while (bVar9 = iVar1 != 0, iVar1 = iVar1 + -1, bVar9) {
    iVar4 = puVar6[5] - uVar8;
    if ((uint)puVar6[5] < uVar8 || iVar4 == 0) {
      uVar3 = puVar6[6] * 2;
      if (0x1800 < uVar3) {
        uVar3 = 0x1800;
      }
      puVar6[6] = uVar3;
      iVar4 = uVar3 + iVar4;
      if (iVar4 < 0) {
        iVar4 = 0;
      }
      puVar6[5] = iVar4;
      FUN_00023360(0x12,*puVar6,puVar6[1],puVar6[2]);
    }
    else {
      puVar6[5] = iVar4;
    }
    puVar6 = puVar6 + 8;
  }
  piVar5 = (int *)*DAT_0002482c;
  do {
    if (piVar5 == (int *)0x0) {
      return;
    }
    uVar8 = *DAT_00024830 - piVar5[7];
    iVar1 = piVar5[1];
    if (iVar1 == 3) {
      iVar1 = 2;
LAB_00023be0:
      if ((uint)piVar5[8] < uVar8) {
        iVar4 = piVar5[9];
        piVar5[9] = iVar4 + 1U;
        if (iVar4 + 1U < 7) {
          uVar8 = piVar5[8] * 2;
          if (0x1800 < uVar8) {
            uVar8 = 0x1800;
          }
          FUN_0002326c(piVar5,iVar1,0);
          piVar5[8] = uVar8;
        }
        else {
          if (iVar1 == 2) {
            uVar2 = 8;
          }
          else {
            uVar2 = 9;
          }
          piVar5[0xf] = uVar2;
          piVar5[1] = 0xc;
        }
      }
    }
    else {
      if (iVar1 != 8 && iVar1 != 5) {
        if (iVar1 == 6) {
          iVar1 = 1;
        }
        else {
          if (iVar1 != 9 && iVar1 != 10) {
            if ((iVar1 == 0xb) && (0x1f400 < uVar8)) {
              piVar5[0xf] = 5;
              piVar5[1] = 0xc;
            }
            goto LAB_00023bb8;
          }
          iVar1 = 0x11;
        }
        goto LAB_00023be0;
      }
      uVar3 = piVar5[0x15] - piVar5[0x14];
      if (uVar3 == 0) {
        if (piVar5[0x10] == 1) {
          FUN_0002326c(piVar5,0x11,0);
          if (piVar5[1] == 8) {
            uVar2 = 10;
          }
          else {
            uVar2 = 6;
          }
          piVar5[1] = uVar2;
          piVar5[0x10] = 2;
        }
      }
      else {
        if ((uint)piVar5[0x15] < (uint)piVar5[0x14]) {
          uVar3 = uVar3 + 0x2000;
        }
        bVar10 = (uint)(piVar5[4] - piVar5[2]) <= uVar3;
        bVar9 = uVar3 == piVar5[4] - piVar5[2];
        if (!bVar10 || bVar9) {
          bVar10 = 0x18 < uVar8;
          bVar9 = uVar8 == 0x19;
        }
        if (bVar10 && !bVar9) {
          uVar8 = piVar5[6] - piVar5[2];
          if (uVar3 < (uint)(piVar5[6] - piVar5[2])) {
            uVar8 = uVar3;
          }
          iVar1 = FUN_0002221c(piVar5[0xb]);
          if (iVar1 - 0x14U < uVar8) {
            uVar8 = iVar1 - 0x14U;
          }
          FUN_0002326c(piVar5,0x10,uVar8);
        }
        else {
          uVar7 = piVar5[8];
          if (uVar7 < uVar8) {
            uVar8 = piVar5[6] - piVar5[2];
            if (uVar3 < (uint)(piVar5[6] - piVar5[2])) {
              uVar8 = uVar3;
            }
            iVar1 = FUN_0002221c(piVar5[0xb]);
            if (iVar1 - 0x14U < uVar8) {
              uVar8 = iVar1 - 0x14U;
            }
            uVar7 = uVar7 * 2;
            if (0x1800 < uVar7) {
              uVar7 = 0x1800;
            }
            FUN_0002326c(piVar5,0x10,uVar8);
            piVar5[8] = uVar7;
          }
        }
      }
    }
LAB_00023bb8:
    piVar5 = (int *)*piVar5;
  } while( true );
}

