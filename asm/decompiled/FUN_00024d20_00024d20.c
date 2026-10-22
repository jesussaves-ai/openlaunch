// Function: FUN_00024d20
// Address: 00024d20


undefined4 FUN_00024d20(int param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  byte *pbVar4;
  
  if (param_2 == 0) {
    uVar1 = 3;
  }
  else if ((*(ushort *)(param_1 + 0x36) & 0x8000) == 0) {
    uVar1 = 2;
  }
  else {
    do {
      iVar2 = FUN_0001c6c8(param_1);
    } while (iVar2 != 0);
    uVar3 = 0;
    iVar2 = 8;
    pbVar4 = (byte *)(param_1 + 0x40);
    do {
      uVar3 = uVar3 + *pbVar4;
      iVar2 = iVar2 + -1;
      pbVar4 = pbVar4 + 1;
    } while (iVar2 != 0);
    *(short *)(param_1 + 0x34) = (short)(uVar3 >> 3);
    FUN_0001c7d0(param_1,param_2,0x60);
    FUN_0001c6e0(param_1);
    uVar1 = 0;
  }
  return uVar1;
}

