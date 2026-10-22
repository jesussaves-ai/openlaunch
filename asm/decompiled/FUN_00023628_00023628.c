// Function: FUN_00023628
// Address: 00023628


int FUN_00023628(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 == 0) || (*(int *)(param_1 + 4) != 2)) ||
     (piVar5 = *(int **)(param_1 + 0x34), piVar5 == (int *)0x0)) {
    FUN_0001cd28();
    iVar2 = 0;
  }
  else {
    iVar2 = *piVar5;
    if (iVar2 == 0) {
      FUN_0001cd68();
      iVar2 = 0;
    }
    else {
      iVar4 = *(int *)(param_1 + 0x38);
      do {
        iVar4 = iVar4 + -1;
        if (iVar4 == 0) {
          iVar3 = 0;
        }
        else {
          iVar3 = piVar5[1];
        }
        *piVar5 = iVar3;
        piVar5 = piVar5 + 1;
      } while (iVar4 != 0 && iVar3 != 0);
    }
  }
  FUN_00020af8(uVar1);
  return iVar2;
}

