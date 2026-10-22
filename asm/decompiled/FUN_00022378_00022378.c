// Function: FUN_00022378
// Address: 00022378


void FUN_00022378(int param_1)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_000248c8(param_1 + 0x20);
  if (piVar1 != (int *)0x0) {
    *piVar1 = param_1;
    piVar1[1] = param_1;
    piVar1[3] = (int)(piVar1 + 8);
    piVar1[2] = 0;
  }
  return;
}

