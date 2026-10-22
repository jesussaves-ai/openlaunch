// Function: FUN_0001d2e4
// Address: 0001d2e4


undefined4 FUN_0001d2e4(void)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined8 uVar4;
  
  uVar1 = FUN_0001cf04(DAT_0001db6c,DAT_0001db70,0xc);
  if (*(ushort *)(DAT_0001db74 + 2) == uVar1) {
    uVar4 = FUN_0001d628(uVar1);
    iVar3 = FUN_0001d5a0((int)uVar4,(int)((ulonglong)uVar4 >> 0x20) + 0x58);
    if (iVar3 == 1) {
      uVar2 = 0;
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 1;
  }
  return uVar2;
}

