// Function: FUN_06024280
// Address:  06024280
// Signature: undefined FUN_06024280()


void FUN_06024280(undefined4 param_1,int param_2,int param_3)

{
  int iVar1;
  
  iVar1 = *DAT_06025090;
  FUN_0602946c();
  do {
    param_2 = param_2 - iVar1;
    if (param_2 < 0) {
      FUN_060242c0();
    }
    *DAT_06025094 = 0;
    FUN_06029590();
  } while (param_3 != 0);
  return;
}

