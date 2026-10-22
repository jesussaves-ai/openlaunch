// Function: FUN_00028b78
// Address: 00028b78


void FUN_00028b78(undefined1 *param_1,int param_2,int param_3)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)(param_2 + param_3);
  do {
    puVar1 = puVar1 + -1;
    *puVar1 = *param_1;
    param_3 = param_3 + -1;
    param_1 = param_1 + 1;
  } while (param_3 != 0);
  return;
}

