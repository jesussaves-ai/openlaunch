// Function: FUN_0001f21c
// Address: 0001f21c


void FUN_0001f21c(void)

{
  byte bVar1;
  ulonglong uVar2;
  undefined1 *puVar3;
  code *pcVar4;
  undefined4 uVar5;
  byte *pbVar6;
  byte *pbVar7;
  uint *puVar8;
  ushort *puVar9;
  ushort *puVar10;
  undefined1 *extraout_r1;
  undefined1 *extraout_r1_00;
  byte *extraout_r1_01;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint *puVar14;
  ushort *puVar15;
  uint uVar16;
  byte *unaff_r7;
  bool bVar17;
  longlong lVar18;
  undefined8 uVar19;
  ushort auStack_30 [3];
  undefined1 auStack_2a [2];
  uint local_28;
  undefined4 local_24;
  uint *local_20;
  ushort *local_1c;
  uint uStack_18;
  byte *local_14;
  
  lVar18 = FUN_00020ae4();
  puVar14 = DAT_0001f884;
  puVar15 = (ushort *)lVar18;
  uVar16 = 10;
LAB_0001f230:
  do {
    uVar16 = uVar16 - 1;
    if (uVar16 == 0) {
LAB_0001f34c:
      FUN_00020af8(puVar15);
      return;
    }
    FUN_0001e8f0((int)lVar18,(int)((ulonglong)lVar18 >> 0x20));
    FUN_0001ef68();
    FUN_0001efa8();
    if (*DAT_0001f888 == '\0') {
      FUN_0001d4f4();
      if ((((*puVar14 & 1) == 0) || ((puVar14[1] & 1) == 0)) ||
         ((*(byte *)((int)puVar14 + 5) & 1) == 0)) goto LAB_0001f34c;
      *DAT_0001f88c = puVar14[2];
    }
    uVar11 = *(ushort *)(DAT_0001f890 + 2) + 0x85;
    FUN_0001d930(DAT_0001f894,DAT_0001f898 - (uVar11 & 0xffffff80),uVar11 >> 7);
    pbVar6 = DAT_0001f89c;
    *DAT_0001f89c = 0;
    if (*(byte *)((int)puVar14 + 1) != 0) {
      if (*(byte *)((int)puVar14 + 1) != 2) goto LAB_0001f660;
      uVar11 = (uint)(byte)puVar14[1];
      if (uVar11 != 0) {
        if (*(ushort *)((int)puVar14 + 2) < uVar11) goto LAB_0001f660;
        pbVar7 = (byte *)((int)puVar14 + (*(ushort *)((int)puVar14 + 2) - uVar11) + 6);
        do {
          uVar12 = uVar11 - 2;
          if (uVar11 < 2) goto LAB_0001f660;
          pbVar6 = pbVar7 + 2;
          uVar13 = (uint)pbVar7[1];
          uVar11 = uVar12 - uVar13;
          if (uVar12 < uVar13) goto LAB_0001f660;
          if (*pbVar7 == 1) {
            do {
              bVar17 = uVar13 < 2;
              uVar13 = uVar13 - 2;
              if (bVar17) goto LAB_0001f660;
              pbVar7 = pbVar6 + 1;
              bVar1 = *pbVar6;
              puVar3 = DAT_0001f8a0;
              if (((bVar1 != 1) && (puVar3 = DAT_0001f8a4, bVar1 != 2)) &&
                 (puVar3 = DAT_0001f8a8, bVar1 != 5)) goto LAB_0001f660;
              *puVar3 = 0;
              pbVar6 = pbVar6 + 2;
              local_14 = pbVar7;
              if (*pbVar7 != 1) goto LAB_0001f660;
            } while (uVar13 != 0);
          }
          else {
            if ((*pbVar7 != 2) || (uVar13 != 6)) goto LAB_0001f660;
            bVar1 = *pbVar6;
            pbVar6 = pbVar7 + 8;
            if (bVar1 == 0xaa && pbVar7[7] == 0x55) {
              *DAT_0001f8ac = *(undefined4 *)(pbVar7 + 3);
              *DAT_0001f8b0 = 1;
            }
            local_24 = 6;
            local_28 = uVar11;
            local_20 = puVar14;
            local_1c = puVar15;
            uStack_18 = uVar16;
            local_14 = unaff_r7;
          }
          pbVar7 = pbVar6;
        } while (uVar11 != 0);
      }
    }
    uVar11 = *puVar14 & 0xff;
    if (uVar11 == 0) {
      if ((*(byte *)((int)puVar14 + 1) != 2) ||
         (lVar18 = CONCAT44(pbVar6,(uint)*(ushort *)((int)puVar14 + 2)),
         (uint)*(ushort *)((int)puVar14 + 2) != (uint)(byte)puVar14[1])) goto LAB_0001f660;
      goto LAB_0001f230;
    }
    if (uVar11 == 1) {
      uVar11 = (uint)*(ushort *)((int)puVar14 + 6);
      puVar8 = puVar14 + 2;
      if (*(byte *)((int)puVar14 + 1) == 2) {
        uVar12 = (uint)(byte)puVar14[1];
      }
      else {
        uVar12 = 0;
      }
      uVar13 = (uint)*(ushort *)((int)puVar14 + 2) - (uVar12 + 2);
      if ((uint)*(ushort *)((int)puVar14 + 2) < uVar12 + 2) goto LAB_0001f660;
      if (uVar11 == 0x1010) {
        uVar11 = *puVar8;
        if ((uVar13 < 4) ||
           ((lVar18 = CONCAT44(puVar14 + 3,uVar11), uVar11 != 0x3007 &&
            (lVar18 = CONCAT44(puVar14 + 3,uVar11), uVar11 != 0x3008)))) goto LAB_0001f660;
      }
      else if (uVar11 == 0x1004) {
        local_24 = CONCAT31(local_24._1_3_,*(byte *)((int)puVar14 + 0xb));
        local_20 = puVar14;
        local_1c = puVar15;
        uStack_18 = uVar16;
        local_14 = unaff_r7;
        FUN_00025f60((byte *)((int)puVar14 + 0xe),auStack_30);
        FUN_00025f60((byte *)((int)puVar14 + 0xe),auStack_2a);
        puVar8 = puVar14 + 6;
        puVar14 = local_20;
        puVar15 = local_1c;
        uVar16 = uStack_18;
        unaff_r7 = local_14;
        lVar18 = CONCAT44(puVar8,auStack_30);
        if (0x10 < uVar13) {
          lVar18 = FUN_00025fac();
          puVar14 = local_20;
          puVar15 = local_1c;
          uVar16 = uStack_18;
          unaff_r7 = local_14;
        }
      }
      else if (uVar11 == 0x100a) {
        *DAT_0001f8dc = 0;
        if (uVar13 != 4) goto LAB_0001f660;
        pcVar4 = (code *)*puVar8;
        lVar18 = CONCAT44(puVar8,pcVar4);
        if (pcVar4 != (code *)0x0 && pcVar4 != DataAbort) {
LAB_0001f660:
          FUN_00029830();
          uVar5 = FUN_000298f8(*DAT_0001f8e0);
          uVar16 = 0;
          FUN_00029830(uVar5,puVar14);
          do {
            FUN_00029830();
            puVar3 = extraout_r1;
            do {
              FUN_000298c8(*puVar3,puVar3 + 1);
              FUN_00029840();
              uVar16 = uVar16 + 1;
              puVar3 = extraout_r1_00;
            } while ((uVar16 & 7) != 0);
          } while (uVar16 != 0x40);
          FUN_00029830();
          FUN_00029830();
          uVar16 = (uint)(byte)puVar14[1];
          if (uVar16 != 0 && uVar16 != 0xff) {
            pbVar6 = (byte *)((int)puVar14 + ((*(ushort *)((int)puVar14 + 2) + 6) - uVar16));
            do {
              FUN_000298c8(*pbVar6,pbVar6 + 1);
              FUN_00029840();
              uVar16 = uVar16 - 1;
              pbVar6 = extraout_r1_01;
            } while (uVar16 != 0);
            FUN_00029830();
          }
          do {
                    /* WARNING: Do nothing block with infinite loop */
          } while( true );
        }
      }
      else if (uVar11 == 0x1002) {
        *DAT_0001f8c4 = 2;
        *DAT_0001f8c8 = 0;
        *(bool *)DAT_0001f8d0 = *DAT_0001f8cc != '\0';
        uVar2 = ZEXT48(DAT_0001f8d4);
        *DAT_0001f8d4 = 0;
        lVar18 = uVar2 << 0x20;
      }
      else if (uVar11 == 0x1003) {
        if ((byte)puVar14[4] != 3) goto LAB_0001f660;
        uVar2 = ZEXT48(DAT_0001f8d8);
        *DAT_0001f8d8 = 0;
        lVar18 = uVar2 << 0x20;
      }
      else {
        lVar18 = CONCAT44(puVar8,uVar11);
        if (((uVar11 != 0x1008) && (lVar18 = CONCAT44(puVar8,uVar11), uVar11 != 0x1007)) &&
           (lVar18 = CONCAT44(puVar8,uVar11), uVar11 != 0x1001)) {
          if (uVar11 == 0x1006) {
            uVar11 = *puVar8;
            *DAT_0001f8c0 = uVar11;
            lVar18 = CONCAT44(puVar8,uVar11);
          }
          else {
            lVar18 = CONCAT44(puVar8,uVar11);
            if ((uVar11 != 0x100d) && (lVar18 = CONCAT44(puVar8,uVar11), uVar11 != 0x1025)) {
              if (uVar11 != 0xe) goto LAB_0001f660;
              puVar9 = (ushort *)((int)puVar14 + 10);
              uVar11 = (uint)*(byte *)((int)puVar14 + 9);
              pbVar6 = (byte *)0x0;
              do {
                puVar10 = puVar9 + 1;
                if (*puVar9 == 0x9b4) {
                  uVar12 = 0xb4;
                  puVar9 = puVar10;
                  pbVar6 = (byte *)((uint)pbVar6 | 0x4000);
                }
                else {
                  uVar12 = *puVar9 - 0x96c;
                  puVar9 = puVar10;
                  if (uVar12 < 0x3d) {
                    local_1c = puVar10;
                    uStack_18 = uVar11;
                    local_14 = pbVar6;
                    uVar19 = FUN_0001cf0c(uVar12,5);
                    uVar12 = 1 << ((int)uVar19 + 1U & 0xff);
                    puVar9 = local_1c;
                    uVar11 = uStack_18;
                    pbVar6 = local_14;
                    if ((int)((ulonglong)uVar19 >> 0x20) == 0) {
                      pbVar6 = (byte *)((uint)local_14 | uVar12);
                    }
                  }
                }
                lVar18 = CONCAT44(DAT_0001f8bc,uVar12);
                uVar11 = uVar11 - 1;
              } while (uVar11 != 0);
              *DAT_0001f8bc = pbVar6;
            }
          }
        }
      }
    }
    else {
      if ((uVar11 != 2) && (uVar11 != 5)) goto LAB_0001f660;
      if (*(ushort *)(DAT_0001f8b4 + 0x1c) == DAT_0001f8b8) {
        lVar18 = FUN_0001e934();
      }
      else {
        lVar18 = FUN_0001f068();
      }
    }
  } while( true );
}

