// Function: FUN_0001c6c8
// Address: 0001c6c8


undefined4 FUN_0001c6c8(int param_1)

{
  int iVar1;
  int *piVar2;
  
  piVar2 = (int *)(param_1 + 0x38);
  if (*piVar2 != 0) {
    return 1;
  }
  iVar1 = *piVar2;
  *piVar2 = 2;
  if (iVar1 != 0 && iVar1 != 2) {
    *piVar2 = iVar1;
    return 1;
  }
  return 0;
}

