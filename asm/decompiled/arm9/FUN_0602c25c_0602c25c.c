// Function: FUN_0602c25c
// Address:  0602c25c
// Signature: undefined FUN_0602c25c()


undefined8 FUN_0602c25c(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  bool bVar4;
  undefined8 uVar5;
  
  iVar1 = DAT_0602c4d8;
  bVar4 = *DAT_0602c4dc == 0xff;
  if ((!bVar4) &&
     (FUN_0602b718(DAT_0602c4d8 + 0xb0,DAT_0602c4e0,0x10,param_4,*DAT_0602c4dc,DAT_0602c4dc,param_3,
                   param_4,param_1,param_2,param_3,param_4), !bVar4)) {
    uVar2 = DAT_0602c4e4;
    uVar3 = DAT_0602c4e8;
    if ((*(byte *)(iVar1 + 0x12) & 2) != 0) {
      uVar2 = DAT_0602c4ec;
      uVar3 = DAT_0602c4f0;
    }
    while (uVar2 < uVar3) {
      FUN_0602c308(uVar2,uVar3);
      FUN_0602c308();
      FUN_0602c308();
      FUN_0602c308();
      uVar5 = FUN_0602c2fc();
      uVar2 = FUN_0602c0bc((int)uVar5,(int)((ulonglong)uVar5 >> 0x20) - (int)uVar5);
    }
  }
  return CONCAT44(param_2,param_1);
}

