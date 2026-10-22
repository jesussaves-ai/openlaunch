// Function: FUN_0001c6e0
// Address: 0001c6e0


undefined4 FUN_0001c6e0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  int *piVar3;
  
  piVar3 = (int *)(param_1 + 0x38);
  if (*piVar3 == 2) {
    iVar1 = *piVar3;
    *piVar3 = 0;
    if (iVar1 == 2) {
      uVar2 = 0;
    }
    else {
      uVar2 = 2;
    }
    return uVar2;
  }
  return 2;
}

