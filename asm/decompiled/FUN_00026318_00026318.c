// Function: FUN_00026318
// Address: 00026318


void FUN_00026318(void)

{
  byte bVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  
  piVar2 = DAT_00026d20;
  iVar4 = *DAT_00026d20;
  while (bVar1 = *(byte *)(DAT_00026d24 + iVar4), bVar1 == 0) {
    uVar3 = *DAT_00026d2c * 2;
    if (7 < uVar3) {
      uVar3 = 7;
    }
    *DAT_00026d2c = uVar3;
    iVar5 = 0x20;
    iVar4 = DAT_00026d30;
    do {
      if (((*(ushort *)(iVar4 + 0x36) & 0x8000) != 0) &&
         (uVar3 = *(int *)(iVar4 + 0x30) + 1, *(uint *)(iVar4 + 0x30) = uVar3, 3 < uVar3)) {
        *(undefined2 *)(iVar4 + 0x34) = 0;
        *(undefined1 *)(iVar4 + 0x40) = 0;
        *(undefined1 *)(iVar4 + 0x41) = 0;
        *(undefined1 *)(iVar4 + 0x42) = 0;
        *(undefined1 *)(iVar4 + 0x43) = 0;
        *(undefined1 *)(iVar4 + 0x44) = 0;
        *(undefined1 *)(iVar4 + 0x45) = 0;
        *(undefined1 *)(iVar4 + 0x46) = 0;
        *(undefined1 *)(iVar4 + 0x47) = 0;
      }
      iVar4 = iVar4 + 0x60;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
    iVar4 = 0;
  }
  *piVar2 = iVar4 + 1;
  *DAT_00026d28 = (ushort)bVar1;
  return;
}

