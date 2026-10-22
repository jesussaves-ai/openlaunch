// Function: FUN_060270d4
// Address:  060270d4
// Signature: undefined FUN_060270d4()


void FUN_060270d4(int param_1)

{
  int *piVar1;
  
  piVar1 = DAT_06027508;
  *DAT_06027508 = param_1;
  FUN_06026324(0xb);
  do {
  } while (*piVar1 != 0);
  return;
}

