// Function: FUN_000218ac
// Address: 000218ac


void FUN_000218ac(void)

{
  byte bVar1;
  int iVar2;
  byte *pbVar3;
  uint uVar4;
  byte *pbVar5;
  byte *pbVar6;
  undefined4 *puVar7;
  uint uVar8;
  int iVar9;
  bool bVar10;
  undefined8 uVar11;
  undefined4 uVar12;
  
  iVar2 = *DAT_00021ccc;
  if (iVar2 == 1) {
    pbVar3 = (byte *)FUN_000248c8(0x224);
    if (pbVar3 == (byte *)0x0) {
      FUN_00021c04();
      iVar2 = 2;
    }
    else {
      uVar12 = 0x10;
LAB_000218e8:
      iVar2 = FUN_00022d08(*DAT_00021cd0,pbVar3,0x224,0,uVar12);
      if (iVar2 != -1) {
        uVar4 = (uint)*pbVar3;
        bVar10 = (uVar4 == 2 && pbVar3[1] == 1) && pbVar3[2] == 6;
        if ((uVar4 == 2 && pbVar3[1] == 1) && pbVar3[2] == 6) {
          uVar4 = *DAT_00021cd4;
          bVar10 = *(uint *)(pbVar3 + 4) == uVar4;
        }
        if (bVar10) {
          if (-1 < iVar2 + -0xf0) {
            uVar11 = FUN_0001c8d8(uVar4,pbVar3 + 0xec);
            pbVar6 = (byte *)((ulonglong)uVar11 >> 0x20);
            iVar2 = iVar2 + -0xf0;
            if ((int)uVar11 == DAT_00021cd8) {
LAB_00021958:
              iVar9 = iVar2;
              pbVar5 = pbVar6;
              if (0 < iVar9) goto code_r0x00021960;
              goto LAB_000219f8;
            }
          }
        }
        goto LAB_000218e8;
      }
      FUN_000249f8(pbVar3);
      iVar2 = *DAT_00021d00;
      if ((uint)(iVar2 - *DAT_00021d04) < 0xb401) {
        if (0xc00 < (uint)(iVar2 - *DAT_00021d08)) {
          FUN_00029878(iVar2,DAT_00021d4c);
LAB_00021bd0:
          if (*DAT_00021d50 == 0) {
            uVar12 = 1;
          }
          else {
            uVar12 = 3;
          }
          FUN_00021558(uVar12);
        }
        iVar2 = 1;
      }
      else {
        FUN_00021c04();
        iVar2 = 2;
      }
    }
  }
LAB_00021a88:
  *DAT_00021d0c = iVar2;
  return;
code_r0x00021960:
  bVar1 = *pbVar5;
  pbVar6 = pbVar5 + 1;
  iVar2 = iVar9 + -1;
  if (bVar1 == 0) goto LAB_00021958;
  if (bVar1 == 0xff) {
LAB_000219f8:
    *DAT_00021ce8 = *(undefined4 *)(pbVar3 + 0x10);
    *DAT_00021cec = 1;
    FUN_00029878(1,DAT_00021cf0);
    *DAT_00021cfc = *DAT_00021cfc + (*DAT_00021cf4 - *DAT_00021cf8) + 1;
    FUN_000249f8(pbVar3);
    goto LAB_00021bd0;
  }
  pbVar6 = pbVar5 + 2;
  uVar4 = (uint)pbVar5[1];
  iVar2 = (iVar9 + -2) - uVar4;
  if (iVar2 < 0) goto LAB_000218e8;
  if (bVar1 == 0x35) {
    if (iVar2 == 0) goto LAB_000218e8;
    if (*DAT_00021d24 == 0) {
      bVar1 = 2;
    }
    else {
      bVar1 = 5;
    }
    if (*pbVar6 != bVar1) goto LAB_000218e8;
    if (*DAT_00021d28 != 0) {
      FUN_000249f8(pbVar3);
      FUN_00021c04();
      iVar2 = *DAT_00021d2c;
      *(undefined4 *)(iVar2 + 0x10) = *DAT_00021d30;
      *(undefined4 *)(iVar2 + 0x14) = *DAT_00021d34;
      *(undefined4 *)(iVar2 + 0x18) = *DAT_00021d38;
      if ((*DAT_00021d3c & 2) != 0) {
        *(undefined4 *)(iVar2 + 0x28) = *DAT_00021d40;
      }
      if ((*DAT_00021d44 & 1) != 0) {
        FUN_0001c7d0(DAT_00021d48,iVar2 + 0x1c,0xc);
      }
      iVar2 = 3;
      goto LAB_00021a88;
    }
  }
  else if (*DAT_00021cdc == 0) {
    if (3 < uVar4) {
      if (bVar1 == 1) {
        uVar11 = FUN_0001c8d8();
        pbVar6 = (byte *)((int)((ulonglong)uVar11 >> 0x20) + -4);
        *DAT_00021d10 = (int)uVar11;
        goto LAB_000219f0;
      }
      if (bVar1 == 3) {
        uVar11 = FUN_0001c8d8();
        pbVar6 = (byte *)((int)((ulonglong)uVar11 >> 0x20) + -4);
        *DAT_00021d14 = (int)uVar11;
        goto LAB_000219f0;
      }
      if (bVar1 == 0x36) {
        uVar11 = FUN_0001c8d8();
        pbVar6 = (byte *)((int)((ulonglong)uVar11 >> 0x20) + -4);
        *DAT_00021d18 = (int)uVar11;
        goto LAB_000219f0;
      }
    }
    if (((*DAT_00021ce0 & 1) == 0) || (bVar1 != 6)) {
      if (((*DAT_00021ce4 & 2) != 0) && (bVar1 == 0x2a)) {
        uVar11 = FUN_0001c8d8();
        puVar7 = DAT_00021d1c;
        uVar11 = FUN_000298f8((int)uVar11,(int)((ulonglong)uVar11 >> 0x20) + -4);
        pbVar6 = (byte *)((ulonglong)uVar11 >> 0x20);
        *puVar7 = (int)uVar11;
      }
    }
    else {
      uVar8 = uVar4 & 0xfffffffc;
      if (0xc < uVar8) {
        uVar8 = 0xc;
      }
      FUN_0001c7d0(pbVar6,DAT_00021d20,uVar8);
    }
  }
LAB_000219f0:
  pbVar6 = pbVar6 + uVar4;
  goto LAB_00021958;
}

