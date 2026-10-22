// Function: FUN_0602a4cc
// Address:  0602a4cc
// Signature: undefined FUN_0602a4cc()


void FUN_0602a4cc(short *param_1,short *param_2)

{
  short sVar1;
  
  do {
    sVar1 = *param_1;
    *param_2 = sVar1;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
  } while (sVar1 != 0);
  return;
}

