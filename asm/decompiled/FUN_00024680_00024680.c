// Function: FUN_00024680
// Address: 00024680


void FUN_00024680(int param_1)

{
  undefined4 uVar1;
  int *piVar2;
  int *piVar3;
  
  if (param_1 != 0) {
    uVar1 = FUN_00020ae4();
    piVar2 = DAT_00024884;
    if (*(int *)(param_1 + 0x14) != 0) {
      FUN_000223c8();
      piVar2 = DAT_00024884;
    }
    do {
      piVar3 = piVar2;
      piVar2 = (int *)*piVar3;
      if (piVar2 == (int *)0x0) {
        return;
      }
    } while (piVar2 != (int *)param_1);
    *piVar3 = *piVar2;
    FUN_000249f8(param_1);
    FUN_00020af8(uVar1);
  }
  return;
}

