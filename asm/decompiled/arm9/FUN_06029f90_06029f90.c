// Function: FUN_06029f90
// Address:  06029f90
// Signature: undefined FUN_06029f90()


undefined8 FUN_06029f90(int param_1,int param_2,uint param_3)

{
  int iVar1;
  int extraout_r1;
  int iVar2;
  int iVar3;
  undefined1 uVar4;
  undefined8 uVar5;
  
  FUN_06028960();
  iVar2 = DAT_0602a394;
  *(short *)(DAT_0602a394 + 10) = (short)(param_3 >> 9);
  *(uint *)(iVar2 + 0x108) = param_3 >> 9;
  *(undefined2 *)(iVar2 + 0x26) = 0x200;
  *(undefined4 *)(iVar2 + 0x104) = 0x200;
  FUN_06028838();
  uVar4 = *(short *)(extraout_r1 + 0x30) == 0;
  iVar2 = param_1;
  if ((bool)uVar4) {
    iVar2 = param_1 << 9;
  }
  uVar5 = FUN_060287b8(iVar2,0x19);
  iVar2 = param_1;
  iVar3 = param_2;
  do {
    FUN_06028cc8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20));
    iVar1 = iVar2;
    if ((bool)uVar4) {
      iVar1 = FUN_0602a114(iVar2,iVar3);
    }
    FUN_0602a0d8(iVar1,iVar3,0x200);
    FUN_06028cc8();
    uVar5 = CONCAT44(iVar3,iVar2);
    if ((bool)uVar4) {
      uVar5 = FUN_0602a114();
    }
    iVar2 = iVar2 + 1;
    iVar3 = iVar3 + 0x200;
    param_3 = param_3 - 0x200;
    uVar4 = param_3 == 0;
  } while (!(bool)uVar4);
  FUN_0602a020();
  return CONCAT44(param_2,param_1);
}

