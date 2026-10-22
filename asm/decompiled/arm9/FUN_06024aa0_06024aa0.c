// Function: FUN_06024aa0
// Address:  06024aa0
// Signature: undefined FUN_06024aa0()


void FUN_06024aa0(uint param_1)

{
  int iVar1;
  bool bVar2;
  undefined8 uVar3;
  
  if (param_1 < 6) {
    FUN_06026324(0x16);
    FUN_06026324(0x28);
    iVar1 = FUN_06028844(param_1 * 0x208 + DAT_06025158);
    if (iVar1 == 0x43) {
      uVar3 = CONCAT44(param_1 * 0x208 + DAT_06025160,0x208);
      do {
        uVar3 = FUN_0602654c((int)uVar3,(int)((ulonglong)uVar3 >> 0x20));
        bVar2 = (uint)uVar3 == 0x10;
        if ((uint)uVar3 < 0x11) break;
        uVar3 = FUN_06026568();
      } while (bVar2);
      FUN_060264e4((int)uVar3,(int)((ulonglong)uVar3 >> 0x20),0x10);
    }
    else {
      FUN_060264a0(iVar1,DAT_0602515c + param_1 * 0x10);
    }
    FUN_06026324(0x29);
  }
  return;
}

