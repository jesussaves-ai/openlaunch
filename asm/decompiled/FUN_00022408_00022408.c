// Function: FUN_00022408
// Address: 00022408


void FUN_00022408(int *param_1,int param_2)

{
  int iVar1;
  
  if (param_1 == (int *)0x0) {
    return;
  }
  param_1[3] = param_1[3] - param_2;
  param_1[1] = param_1[1] + param_2;
  iVar1 = *param_1;
  do {
    *param_1 = iVar1 + param_2;
    param_1 = (int *)param_1[2];
  } while (param_1 != (int *)0x0);
  return;
}

