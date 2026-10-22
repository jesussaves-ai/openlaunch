// Function: FUN_0602a4b8
// Address:  0602a4b8
// Signature: undefined FUN_0602a4b8()


void FUN_0602a4b8(short *param_1,short *param_2)

{
  short sVar1;
  short *psVar2;
  
  do {
    psVar2 = param_2;
    param_2 = psVar2 + 1;
  } while (*psVar2 != 0);
  *psVar2 = 0x2f;
  psVar2 = psVar2 + 1;
  do {
    sVar1 = *param_1;
    *psVar2 = sVar1;
    param_1 = param_1 + 1;
    psVar2 = psVar2 + 1;
  } while (sVar1 != 0);
  return;
}

