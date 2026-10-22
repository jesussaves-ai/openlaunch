// Function: FUN_00022384
// Address: 00022384


void FUN_00022384(int param_1,int param_2)

{
  int *piVar1;
  
  piVar1 = (int *)FUN_000248c8(param_2 + 0x20);
  if (piVar1 != (int *)0x0) {
    *piVar1 = param_1 + param_2;
    piVar1[1] = param_1 + param_2;
    piVar1[3] = (int)piVar1 + (0x20 - param_1);
    piVar1[2] = 0;
  }
  return;
}

