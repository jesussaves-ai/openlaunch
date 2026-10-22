// Function: FUN_0602aa84
// Address:  0602aa84
// Signature: undefined FUN_0602aa84()


void FUN_0602aa84(int param_1)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  bool bVar6;
  
  if ((*(byte *)(param_1 + 3) & 0x10) != 0) {
    return;
  }
  if ((*(byte *)(param_1 + 2) & 1) != 0) {
    return;
  }
  *(byte *)(param_1 + 2) = *(byte *)(param_1 + 2) | 1;
  iVar2 = DAT_0602ae00;
  iVar3 = FUN_06028844(param_1 + 0x34);
  uVar4 = DAT_0602ae04;
  if ((((iVar3 != 0x55) && (uVar4 = DAT_0602ae08, iVar3 != 0x56)) &&
      (uVar4 = DAT_0602ae0c, iVar3 != 0x57)) &&
     ((uVar4 = DAT_0602ae10, iVar3 != 0x58 && (uVar4 = DAT_0602ae14, iVar3 != 0x59)))) {
    bVar6 = iVar3 == 0x5a;
    if (!bVar6) {
      FUN_060288d4();
      if (bVar6) {
        FUN_0602946c(*(undefined4 *)(param_1 + 8));
        FUN_06029f04(*DAT_0602ae18,iVar2,0x400);
        if (*(int *)(iVar2 + 0x100) != DAT_0602ab38) goto LAB_0602abac;
      }
      FUN_0602c104(iVar2,0x400);
      goto LAB_0602abac;
    }
    iVar3 = *DAT_0602ae1c;
    bVar6 = iVar3 == 1 || iVar3 == 2;
    uVar4 = DAT_0602ae20;
    if ((iVar3 != 1 && iVar3 != 2) && (FUN_0602b750(), uVar4 = DAT_0602ae24, bVar6)) {
      FUN_0602b760();
      FUN_0602c1a8(DAT_0602ae28,iVar2,0x400);
      goto LAB_0602abac;
    }
  }
  FUN_0602c104(iVar2,0x400);
  FUN_0602c1a8(uVar4,iVar2,0xc);
LAB_0602abac:
  FUN_0602c1a8(iVar2,param_1 + 0xc,0xc);
  *(undefined2 *)(param_1 + 0x18) = *(undefined2 *)(iVar2 + 0x10);
  bVar1 = *(byte *)(iVar2 + 0x12);
  *(byte *)(param_1 + 0x1a) = bVar1;
  *(undefined1 *)(param_1 + 0x1b) = *(undefined1 *)(iVar2 + 0x1d);
  *(undefined4 *)(param_1 + 0x1c) = *(undefined4 *)(iVar2 + 0x230);
  *(undefined4 *)(param_1 + 0x20) = *(undefined4 *)(iVar2 + 0x234);
  bVar6 = (bVar1 & 2) == 0;
  if (bVar6) {
    uVar4 = 0;
  }
  else {
    uVar4 = *(undefined4 *)(iVar2 + 0x238);
  }
  *(undefined4 *)(param_1 + 0x24) = uVar4;
  if (!bVar6) {
    uVar4 = *(undefined4 *)(iVar2 + 0x23c);
  }
  *(undefined4 *)(param_1 + 0x28) = uVar4;
  uVar5 = *(uint *)(iVar2 + 0x68);
  if (*(uint *)(param_1 + 4) <= uVar5) {
    uVar5 = 0;
  }
  *(uint *)(param_1 + 0x2c) = uVar5;
  *(undefined4 *)(param_1 + 0x30) = 0;
  return;
}

