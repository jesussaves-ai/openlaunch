// Function: FUN_06029b5c
// Address:  06029b5c
// Signature: undefined FUN_06029b5c()


undefined8 FUN_06029b5c(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int extraout_r1;
  uint uVar4;
  int iVar5;
  int iVar6;
  undefined1 uVar7;
  
  iVar3 = DAT_0602a390;
  iVar1 = DAT_0602a2dc;
  iVar2 = *DAT_0602a2d8;
  uVar4 = *DAT_0602a2e0 + DAT_0602a38c & ~DAT_0602a38c;
  *(short *)(DAT_0602a390 + 10) = (short)(uVar4 >> 9);
  *(uint *)(iVar3 + 0x108) = uVar4 >> 9;
  *(undefined2 *)(iVar3 + 0x26) = 0x200;
  *(undefined4 *)(iVar3 + 0x104) = 0x200;
  FUN_06028838();
  uVar7 = *(short *)(extraout_r1 + 0x30) == 0;
  iVar3 = iVar2;
  if ((bool)uVar7) {
    iVar3 = iVar2 << 9;
  }
  iVar3 = FUN_060287b8(iVar3,0x12);
  iVar5 = iVar2;
  iVar6 = iVar1;
  do {
    FUN_0602a04c(iVar3,iVar6,0x200);
    FUN_06028cc8();
    iVar3 = iVar5;
    if ((bool)uVar7) {
      iVar3 = FUN_0602a114(iVar5,iVar6);
    }
    iVar5 = iVar5 + 1;
    iVar6 = iVar6 + 0x200;
    uVar4 = uVar4 - 0x200;
    uVar7 = uVar4 == 0;
  } while (!(bool)uVar7);
  FUN_0602a020();
  return CONCAT44(iVar1,iVar2);
}

