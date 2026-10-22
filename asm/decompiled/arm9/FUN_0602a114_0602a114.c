// Function: FUN_0602a114
// Address:  0602a114
// Signature: undefined FUN_0602a114()


undefined8 FUN_0602a114(int param_1,undefined4 param_2)

{
  uint uVar1;
  bool bVar2;
  undefined4 *puVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 uVar7;
  uint uVar8;
  undefined4 uVar9;
  uint uVar10;
  undefined4 uVar11;
  
  puVar3 = DAT_0602a3a8;
  if (param_1 == 1) {
    return CONCAT44(param_2,1);
  }
  if (param_1 == DAT_0602a3a4) {
    return CONCAT44(param_2,param_1);
  }
  uVar1 = param_1 * 0x20;
  *DAT_0602a3a8 = 0;
  *puVar3 = 0xc00;
  puVar3[1] = 0x200000;
  uVar5 = *DAT_0602a3ac;
  uVar6 = DAT_0602a3ac[1];
  uVar8 = DAT_0602a3ac[2];
  uVar10 = DAT_0602a3ac[3];
  bVar2 = CARRY4(uVar6,(uint)CARRY4(uVar5,uVar1));
  puVar3[8] = uVar5 + uVar1;
  puVar3[9] = uVar6 + CARRY4(uVar5,uVar1);
  puVar3[10] = uVar8 + bVar2;
  puVar3[0xb] = uVar10 + CARRY4(uVar8,(uint)bVar2);
  iVar4 = DAT_0602a3b0;
  *(undefined4 *)(DAT_0602a3b0 + 4) = param_2;
  *(undefined4 **)(iVar4 + 8) = puVar3 + 2;
  *(undefined4 *)(iVar4 + 0xc) = 0x80;
  *(undefined4 *)(iVar4 + 0x10) = 8;
  *(undefined4 *)(iVar4 + 0x14) = 0;
  *(undefined4 *)(iVar4 + 0x1c) = DAT_0602a3b4;
  *(undefined4 **)(iVar4 + 0x20) = puVar3 + 3;
  *(undefined4 *)(iVar4 + 0x24) = param_2;
  *(undefined4 *)(iVar4 + 0x28) = 0x80;
  *(undefined4 *)(iVar4 + 0x2c) = 8;
  *(undefined4 *)(iVar4 + 0x30) = 0;
  *(undefined4 *)(iVar4 + 0x38) = DAT_0602a3b8;
  uVar7 = DAT_0602a3bc[1];
  uVar9 = DAT_0602a3bc[2];
  uVar11 = DAT_0602a3bc[3];
  puVar3[0x3c] = *DAT_0602a3bc;
  puVar3[0x3d] = uVar7;
  puVar3[0x3e] = uVar9;
  puVar3[0x3f] = uVar11;
  *puVar3 = 0xd000000;
  *puVar3 = 0x20000000;
  *puVar3 = 0xa0006000;
  do {
  } while ((*(uint *)(DAT_0602a3c0 + 0x38) & 0x80000000) != 0);
  return CONCAT44(param_2,param_1);
}

