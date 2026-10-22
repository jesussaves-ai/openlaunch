// Function: FUN_06027240
// Address:  06027240
// Signature: undefined FUN_06027240()


undefined8 FUN_06027240(undefined4 param_1,undefined4 param_2)

{
  bool bVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  int iVar11;
  uint *unaff_r7;
  uint uVar12;
  uint *unaff_r8;
  uint *unaff_r9;
  
  uVar6 = *unaff_r8;
  uVar10 = *unaff_r7;
  if (((uVar10 <= uVar6) &&
      (iVar11 = unaff_r7[2] + (uVar6 - uVar10),
      uVar6 + unaff_r8[1] <= uVar10 + (unaff_r7[3] + 0x1f & 0xffffffe0))) &&
     (uVar6 = unaff_r8[1] + 0xf >> 4, uVar6 != 0)) {
    uVar10 = 0;
    do {
      puVar2 = DAT_06027544;
      uVar12 = uVar6;
      if (0x1000 < uVar6) {
        uVar12 = 0x1000;
      }
      uVar6 = uVar6 - uVar12;
      *DAT_06027544 = 0;
      *puVar2 = 0xc00;
      puVar2[1] = uVar12 << 0x10;
      uVar5 = *unaff_r9;
      uVar7 = unaff_r9[1];
      uVar8 = unaff_r9[2];
      uVar9 = unaff_r9[3];
      bVar1 = CARRY4(uVar7,(uint)CARRY4(uVar5,uVar10));
      puVar2[8] = uVar5 + uVar10;
      puVar2[9] = uVar7 + CARRY4(uVar5,uVar10);
      puVar2[10] = uVar8 + bVar1;
      puVar2[0xb] = uVar9 + CARRY4(uVar8,(uint)bVar1);
      uVar4 = DAT_0602754c;
      iVar3 = DAT_06027548;
      *(int *)(DAT_06027548 + 4) = iVar11;
      *(undefined4 *)(iVar3 + 8) = uVar4;
      *(uint *)(iVar3 + 0xc) = uVar12 << 2;
      *(undefined4 *)(iVar3 + 0x10) = 4;
      *(undefined4 *)(iVar3 + 0x14) = 0;
      *(undefined4 *)(iVar3 + 0x1c) = DAT_06027550;
      *(undefined4 *)(iVar3 + 0x20) = DAT_06027554;
      *(int *)(iVar3 + 0x24) = iVar11;
      *(uint *)(iVar3 + 0x28) = uVar12 << 2;
      *(undefined4 *)(iVar3 + 0x2c) = 4;
      *(undefined4 *)(iVar3 + 0x30) = 0;
      *(undefined4 *)(iVar3 + 0x38) = DAT_06027558;
      iVar11 = iVar11 + uVar12 * 0x10;
      *puVar2 = 0x1000000;
      *puVar2 = 0x20000000;
      *puVar2 = 0xa0003000;
      do {
      } while ((*(uint *)(DAT_0602755c + 0x38) & 0x80000000) != 0);
      uVar10 = uVar10 + 0x1000;
    } while (uVar6 != 0);
  }
  return CONCAT44(param_2,param_1);
}

