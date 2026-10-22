// Function: FUN_0001ef68
// Address: 0001ef68


void FUN_0001ef68(void)

{
  int *piVar1;
  undefined4 *puVar2;
  code *UNRECOVERED_JUMPTABLE;
  
  piVar1 = DAT_0001f830;
  puVar2 = (undefined4 *)*DAT_0001f830;
  if (puVar2 == (undefined4 *)0x0) {
    return;
  }
  if (*DAT_0001f834 != '\0' || *DAT_0001f838 != '\0') {
    return;
  }
  UNRECOVERED_JUMPTABLE = (code *)*puVar2;
  *DAT_0001f830 = (int)(puVar2 + 1);
                    /* WARNING: Could not recover jumptable at 0x0001ef9c. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  if (UNRECOVERED_JUMPTABLE != (code *)0x0) {
    (*UNRECOVERED_JUMPTABLE)();
    return;
  }
  *piVar1 = 0;
  return;
}

