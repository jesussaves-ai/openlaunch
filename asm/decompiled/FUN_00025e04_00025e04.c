// Function: FUN_00025e04
// Address: 00025e04


undefined4 FUN_00025e04(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_0002596c(2);
  if (iVar1 == 0) {
LAB_00025e7c:
    uVar2 = 0;
  }
  else {
    FUN_00020cd0();
    FUN_00024bd8(DAT_00025e90);
    FUN_00020be4(4,3);
    while (iVar1 = FUN_000259e4(), iVar1 == 0) {
      FUN_0001cef4();
    }
    if (param_1 != 0) {
      FUN_00024c60();
      while (iVar1 = FUN_000253fc(), iVar1 != 5) {
        if (iVar1 == 6) goto LAB_00025e7c;
        FUN_0001cef4();
      }
    }
    uVar2 = 1;
  }
  return uVar2;
}

