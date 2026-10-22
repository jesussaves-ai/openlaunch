// Function: FUN_00025f7c
// Address: 00025f7c


bool FUN_00025f7c(short *param_1,short *param_2)

{
  short sVar1;
  short sVar2;
  bool bVar3;
  bool bVar4;
  
  sVar1 = *param_1;
  sVar2 = *param_2;
  bVar3 = sVar1 == sVar2;
  if (bVar3) {
    sVar1 = param_1[1];
    sVar2 = param_2[1];
  }
  bVar4 = bVar3 && sVar1 == sVar2;
  if (bVar3 && sVar1 == sVar2) {
    bVar4 = param_1[2] == param_2[2];
  }
  return bVar4;
}

