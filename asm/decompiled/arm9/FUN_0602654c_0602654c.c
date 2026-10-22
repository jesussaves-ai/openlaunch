// Function: FUN_0602654c
// Address:  0602654c
// Signature: undefined FUN_0602654c()


undefined8 FUN_0602654c(undefined4 param_1,short *param_2)

{
  int iVar1;
  short *psVar2;
  
  iVar1 = 0;
  psVar2 = param_2;
  while (*psVar2 != 0) {
    iVar1 = iVar1 + 1;
    psVar2 = psVar2 + 1;
  }
  return CONCAT44(param_2,iVar1);
}

