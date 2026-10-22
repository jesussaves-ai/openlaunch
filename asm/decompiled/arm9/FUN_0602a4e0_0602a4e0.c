// Function: FUN_0602a4e0
// Address:  0602a4e0
// Signature: undefined FUN_0602a4e0()


void FUN_0602a4e0(byte *param_1,ushort *param_2)

{
  byte bVar1;
  
  do {
    bVar1 = *param_1;
    *param_2 = (ushort)bVar1;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
  } while (bVar1 != 0);
  return;
}

