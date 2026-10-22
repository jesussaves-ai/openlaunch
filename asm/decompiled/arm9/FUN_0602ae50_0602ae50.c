// Function: FUN_0602ae50
// Address:  0602ae50
// Signature: undefined FUN_0602ae50()


undefined8 FUN_0602ae50(undefined4 param_1,undefined4 param_2)

{
  ushort *puVar1;
  undefined4 uVar2;
  
  puVar1 = DAT_0602b55c;
  *DAT_0602b55c = *DAT_0602b55c & 0x7fff;
  *puVar1 = *puVar1 | 0x1000;
  *puVar1 = 0x100;
  *(ushort *)(DAT_0602b560 + 4) = *(ushort *)(DAT_0602b560 + 4) | 0x100;
  uVar2 = DAT_0602b56c;
  *DAT_0602b564 = (short)DAT_0602b56c;
  FUN_06026f34(uVar2,DAT_0602b570);
  if ((*(uint *)(DAT_0602b574 + 0x1bc) & 0x1000000) == 0) {
    FUN_06026f34(*(uint *)(DAT_0602b574 + 0x1bc),DAT_0602b578);
  }
  *DAT_0602b57c = (short)DAT_0602b580;
  *(short *)(DAT_0602b584 + 2) = (short)DAT_0602b588;
  return CONCAT44(param_2,param_1);
}

