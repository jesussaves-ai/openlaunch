// Function: FUN_0001c6d4
// Address: 0001c6d4


undefined4 FUN_0001c6d4(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  
  piVar3 = (int *)(param_1 + 0x38);
  if (*piVar3 == 1) {
    iVar1 = *piVar3;
    *piVar3 = 0;
    if (iVar1 == 1) {
      uVar2 = 0;
    }
    else {
      uVar2 = 2;
    }
    return uVar2;
  }
  return 2;
}

