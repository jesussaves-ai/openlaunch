// Function: FUN_06029f00
// Address:  06029f00
// Signature: undefined FUN_06029f00()


undefined8 FUN_06029f00(int param_1,int param_2)

{
  int iVar1;
  int extraout_r1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined1 uVar5;
  
  iVar1 = DAT_0602a390;
  uVar2 = DAT_0602a38c + 0x200 & ~DAT_0602a38c;
  *(short *)(DAT_0602a390 + 10) = (short)(uVar2 >> 9);
  *(uint *)(iVar1 + 0x108) = uVar2 >> 9;
  *(undefined2 *)(iVar1 + 0x26) = 0x200;
  *(undefined4 *)(iVar1 + 0x104) = 0x200;
  FUN_06028838();
  uVar5 = *(short *)(extraout_r1 + 0x30) == 0;
  iVar1 = param_1;
  if ((bool)uVar5) {
    iVar1 = param_1 << 9;
  }
  iVar1 = FUN_060287b8(iVar1,0x12);
  iVar3 = param_1;
  iVar4 = param_2;
  do {
    FUN_0602a04c(iVar1,iVar4,0x200);
    FUN_06028cc8();
    iVar1 = iVar3;
    if ((bool)uVar5) {
      iVar1 = FUN_0602a114(iVar3,iVar4);
    }
    iVar3 = iVar3 + 1;
    iVar4 = iVar4 + 0x200;
    uVar2 = uVar2 - 0x200;
    uVar5 = uVar2 == 0;
  } while (!(bool)uVar5);
  FUN_0602a020();
  return CONCAT44(param_2,param_1);
}

