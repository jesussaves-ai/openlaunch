// Function: FUN_06026568
// Address:  06026568
// Signature: undefined FUN_06026568()


undefined8 FUN_06026568(undefined4 param_1,short *param_2)

{
  short sVar1;
  short *psVar2;
  short *psVar4;
  short *psVar3;
  
  psVar2 = param_2;
  do {
    psVar3 = psVar2 + 1;
    sVar1 = *psVar2;
    psVar4 = psVar3;
    if ((sVar1 == 0x2f) || (sVar1 == 0x5c)) break;
    psVar4 = param_2;
    psVar2 = psVar3;
  } while (sVar1 != 0);
  return CONCAT44(psVar4,param_1);
}

