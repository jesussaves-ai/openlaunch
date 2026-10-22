// Function: FUN_0602ac1c
// Address:  0602ac1c
// Signature: undefined FUN_0602ac1c()


void FUN_0602ac1c(int param_1)

{
  int *piVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  bool bVar5;
  
  if ((((*(byte *)(param_1 + 3) & 0x10) == 0) && (*(int *)(param_1 + 0x2c) != 0)) &&
     (*(int *)(param_1 + 0x30) == 0)) {
    iVar2 = FUN_06028844(param_1 + 0x34);
    if (((((iVar2 != 0x55) && (iVar2 != 0x56)) &&
         ((iVar2 != 0x57 && ((iVar2 != 0x58 && (iVar2 != 0x59)))))) &&
        (bVar5 = iVar2 == 0x5a, !bVar5)) &&
       (FUN_060288d4(iVar2,DAT_0602ae3c), piVar1 = DAT_0602ae40, bVar5)) {
      uVar3 = *(undefined4 *)(param_1 + 8);
      uVar4 = *(undefined4 *)(param_1 + 0x2c);
      iVar2 = *DAT_0602ae40;
      *(int *)(param_1 + 0x30) = iVar2;
      *piVar1 = iVar2 + 0x440;
      FUN_06024280(uVar3,uVar4);
    }
  }
  return;
}

