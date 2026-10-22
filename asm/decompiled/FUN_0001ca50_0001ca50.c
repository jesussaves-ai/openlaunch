// Function: FUN_0001ca50
// Address: 0001ca50


int FUN_0001ca50(char *param_1)

{
  int iVar1;
  
  iVar1 = 0;
  while (*param_1 != '\0') {
    iVar1 = iVar1 + 1;
    param_1 = param_1 + 1;
  }
  return iVar1;
}

