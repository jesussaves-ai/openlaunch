// Function: FUN_060277a8
// Address:  060277a8
// Signature: undefined FUN_060277a8()


undefined4 FUN_060277a8(void)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  undefined8 uVar4;
  
  uVar1 = FUN_0602bf64(DAT_06028250,DAT_06028254,0xc);
  if (*(ushort *)(DAT_06028258 + 2) == uVar1) {
    uVar4 = FUN_06027c28(uVar1);
    iVar3 = FUN_06027b04((int)uVar4,(int)((ulonglong)uVar4 >> 0x20) + 0x58);
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

