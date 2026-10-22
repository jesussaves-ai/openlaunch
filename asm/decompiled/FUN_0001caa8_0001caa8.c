// Function: FUN_0001caa8
// Address: 0001caa8


void FUN_0001caa8(char *param_1,char *param_2,int param_3)

{
  if (param_3 == 0) {
    return;
  }
  do {
    if (*param_1 != *param_2) {
      return;
    }
    param_3 = param_3 + -1;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
  } while (param_3 != 0);
  return;
}

