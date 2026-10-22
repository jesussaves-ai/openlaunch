// Function: FUN_000290d0
// Address: 000290d0


void FUN_000290d0(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined1 uVar3;
  bool bVar4;
  undefined1 uVar5;
  
  iVar1 = *DAT_00029b80 + -1;
  bVar4 = iVar1 < 0;
  if (bVar4) {
    iVar1 = *DAT_00029b80 + 0x3b;
  }
  *DAT_00029b80 = iVar1;
  if (bVar4) {
    *DAT_00029b84 = 1;
    uVar5 = *DAT_00029b88 == '\0';
    uVar3 = true;
    if ((bool)uVar5) {
      uVar2 = FUN_00029184(0,0x59);
      if ((bool)uVar3 && !(bool)uVar5) {
        uVar2 = FUN_00029184(uVar2,0x59);
      }
      if ((bool)uVar3 && !(bool)uVar5) {
        uVar2 = FUN_00029184(uVar2,0x23);
      }
      if ((bool)uVar3 && !(bool)uVar5) {
        FUN_00029184(uVar2,6);
        FUN_00029188(1,0x31);
        if ((bool)uVar3 && !(bool)uVar5) {
          FUN_00029188(1,0x12);
        }
        if ((bool)uVar3 && !(bool)uVar5) {
          FUN_00029184(DAT_00029ba4,0x99);
        }
        if ((bool)uVar3 && !(bool)uVar5) {
          FUN_00029184(DAT_00029ba8,0x99);
        }
      }
    }
    *DAT_00029bac = 0;
  }
  return;
}

