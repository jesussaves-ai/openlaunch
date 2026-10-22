// Function: FUN_06027c34
// Address:  06027c34
// Signature: undefined FUN_06027c34()


undefined8 FUN_06027c34(int param_1)

{
  int *piVar1;
  
  piVar1 = (int *)(*DAT_06028340 + 4 + param_1 * 0x10);
  return CONCAT44(piVar1[3],*piVar1 + *DAT_06028340);
}

