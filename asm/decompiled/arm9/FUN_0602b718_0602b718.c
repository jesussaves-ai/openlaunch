// Function: FUN_0602b718
// Address:  0602b718
// Signature: undefined FUN_0602b718()


void FUN_0602b718(char *param_1,char *param_2,int param_3)

{
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

