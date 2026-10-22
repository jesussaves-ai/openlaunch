// Function: FUN_00026c28
// Address: 00026c28


void FUN_00026c28(undefined2 *param_1,int param_2,int param_3,int param_4)

{
  undefined2 *puVar1;
  
  puVar1 = (undefined2 *)(param_2 + param_3 + DAT_00026e5c);
  do {
    *puVar1 = *param_1;
    param_4 = param_4 + -2;
    param_1 = param_1 + 1;
    puVar1 = puVar1 + 1;
  } while (param_4 != 0);
  return;
}

