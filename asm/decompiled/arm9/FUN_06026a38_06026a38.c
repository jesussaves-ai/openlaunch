// Function: FUN_06026a38
// Address:  06026a38
// Signature: undefined FUN_06026a38()


void FUN_06026a38(short *param_1,char *param_2)

{
  char cVar1;
  byte bVar2;
  short *psVar3;
  short *psVar4;
  
  do {
    while( true ) {
      cVar1 = (char)*param_1;
      if (cVar1 != '*') break;
      psVar3 = param_1 + 2;
      bVar2 = *(byte *)((int)param_1 + 3);
      psVar4 = psVar3;
      if (bVar2 < 0x65) {
        psVar4 = DAT_06027428;
      }
      if (bVar2 == 0x65) {
        psVar4 = DAT_0602742c;
      }
      if (0x65 < bVar2) {
        psVar4 = DAT_06027430;
      }
      while( true ) {
        param_1 = psVar3;
        if (*psVar4 == 0) break;
        *param_2 = (char)*psVar4;
        psVar4 = psVar4 + 1;
        param_2 = param_2 + 1;
      }
    }
    *param_2 = cVar1;
    param_1 = (short *)((int)param_1 + 1);
    param_2 = param_2 + 1;
  } while (cVar1 != '\0');
  return;
}

