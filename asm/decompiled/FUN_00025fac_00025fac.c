// Function: FUN_00025fac
// Address: 00025fac


void FUN_00025fac(int param_1,int param_2,uint param_3)

{
  undefined1 uVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  undefined1 *extraout_r1;
  byte bVar5;
  uint uVar6;
  ushort uVar7;
  byte *pbVar8;
  byte *pbVar9;
  byte *pbVar10;
  int iVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  int iVar15;
  int iVar16;
  byte *pbVar17;
  byte *pbVar18;
  uint uVar19;
  bool bVar20;
  uint local_30;
  uint local_2c;
  
  uVar14 = DAT_00026d04;
  if (9 < param_3) {
    iVar3 = FUN_00025f7c(param_1,param_1 + 6);
    if (iVar3 == 0) {
      uVar14 = uVar14 | 1;
    }
    uVar12 = param_3 - 0xc;
    if (1 < param_3 - 10) {
      if ((*(ushort *)(param_2 + 10) & 0x10) != 0) {
        uVar14 = uVar14 | 2;
      }
      pbVar17 = (byte *)0x0;
      pbVar18 = (byte *)0x0;
      uVar19 = (uint)*DAT_00026d08;
      local_30 = 0;
      local_2c = 0;
      pbVar8 = (byte *)(param_2 + 0xc);
LAB_00026014:
      uVar13 = uVar12 - 2;
      if (1 < uVar12) {
        bVar5 = *pbVar8;
        pbVar9 = pbVar8 + 2;
        uVar6 = (uint)pbVar8[1];
        pbVar10 = pbVar9 + uVar6;
        uVar12 = uVar13 - uVar6;
        if (uVar13 < uVar6) goto LAB_00026060;
        if (bVar5 == 0) {
          pbVar17 = pbVar8 + 1;
          pbVar8 = pbVar10;
        }
        else {
          if (bVar5 != 1) {
            if (bVar5 == 3) {
              pbVar8 = pbVar10;
              if (uVar6 != 0) {
                uVar19 = (uint)*pbVar9;
              }
              goto LAB_00026014;
            }
            if (bVar5 == 0x30) {
              uVar13 = FUN_00026260();
              local_2c = uVar13;
            }
            else {
              pbVar8 = pbVar10;
              if (((bVar5 != 0xdd) || (uVar6 < 4)) ||
                 (iVar3 = FUN_0001c8d8(0xdd,pbVar9,uVar6 - 4), iVar3 != DAT_00026d18))
              goto LAB_00026014;
              uVar13 = FUN_00026260();
              local_30 = uVar13;
            }
            if ((uVar13 & 0x60) != 0) {
              uVar14 = uVar14 | 4;
            }
            pbVar8 = pbVar10;
            if ((uVar13 & 0x10) != 0) {
              uVar14 = uVar14 | 2;
            }
            goto LAB_00026014;
          }
          pbVar18 = pbVar8 + 1;
          pbVar8 = pbVar10;
        }
        goto LAB_00026014;
      }
LAB_00026060:
      iVar11 = 0x20;
      uVar12 = 0;
      iVar3 = DAT_00026d0c;
      iVar16 = DAT_00026d0c;
      do {
        iVar4 = FUN_00025f7c(iVar3 + 0x22,param_1 + 6);
        iVar15 = iVar3;
        if (iVar4 != 0) break;
        if ((*(ushort *)(iVar3 + 0x36) & 0x8000) == 0) {
          uVar13 = 0xffffffff;
        }
        else {
          uVar13 = *(uint *)(iVar3 + 0x30);
        }
        iVar15 = iVar16;
        if (uVar12 < uVar13) {
          iVar15 = iVar3;
          uVar12 = uVar13;
        }
        iVar3 = iVar3 + 0x60;
        iVar11 = iVar11 + -1;
        iVar16 = iVar15;
      } while (iVar11 != 0);
      iVar3 = FUN_0001c6bc(iVar15);
      if (iVar3 == 0) {
        if (pbVar17 != (byte *)0x0) {
          bVar5 = *pbVar17;
          if (bVar5 != 0) {
            if (0x20 < bVar5) {
              bVar5 = 0x20;
            }
            *(byte *)(iVar15 + 0x21) = bVar5;
            FUN_0001c7d0(pbVar17 + 1,iVar15);
            *extraout_r1 = 0;
            *(undefined4 *)(iVar15 + 0x30) = 0;
            FUN_00025f60(param_1,iVar15 + 0x28);
            FUN_00025f60(param_1 + 6,iVar15 + 0x22);
            uVar7 = 0;
            uVar12 = 0x48;
            if (pbVar18 != (byte *)0x0) {
              uVar14 = uVar14 & 0xffffffe7;
              uVar13 = (uint)*pbVar18;
              uVar7 = 0;
              while( true ) {
                pbVar18 = pbVar18 + 1;
                bVar20 = uVar13 == 0;
                uVar13 = uVar13 - 1;
                if (bVar20) break;
                bVar5 = *pbVar18;
                uVar2 = (ushort)bVar5;
                if ((bVar5 & 0x80) != 0) {
                  if (uVar12 < 0x57) {
                    *(byte *)(iVar15 + uVar12) = bVar5;
                    uVar12 = uVar12 + 1;
                  }
                  uVar14 = uVar14 & 0xffffffe7;
                  if (uVar2 == 0x82 || uVar2 == 0x84) {
                    uVar14 = uVar14 | 8;
                  }
                  if (uVar2 == 0x8b || uVar2 == 0x96) {
                    uVar14 = uVar14 | 0x10;
                  }
                }
                if (uVar7 < (uVar2 & 0x7f)) {
                  uVar7 = uVar2 & 0x7f;
                }
              }
            }
            uVar2 = (ushort)uVar14;
            *(undefined1 *)(iVar15 + uVar12) = 0;
            *(ushort *)(iVar15 + 0x2e) = uVar7;
            if (iVar11 == 0) {
              *(undefined4 *)(iVar15 + 0x40) = 0;
              *(undefined4 *)(iVar15 + 0x44) = 0;
            }
            if (*DAT_00026d10 == uVar19) {
              FUN_0001c7d0(iVar15 + 0x41,iVar15 + 0x40,7);
              uVar1 = *(undefined1 *)(param_1 + 0xc);
              *(undefined1 *)(iVar15 + 0x47) = uVar1;
              if (*(char *)(iVar15 + 0x40) == '\0') {
                FUN_0001c714(uVar1,iVar15 + 0x40);
              }
            }
            if ((uVar14 & 4) != 0) {
              uVar2 = uVar2 & 0xffe7;
            }
            *(ushort *)(iVar15 + 0x36) = uVar2;
            *(char *)(iVar15 + 0x3c) = (char)uVar19;
            *(uint *)(iVar15 + 0x58) = local_30;
            *(uint *)(iVar15 + 0x5c) = local_2c;
          }
        }
        FUN_0001c6d4(iVar15);
      }
    }
  }
  return;
}

