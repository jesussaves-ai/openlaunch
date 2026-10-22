// Function: FUN_0602a3dc
// Address:  0602a3dc
// Signature: undefined FUN_0602a3dc()


/* WARNING: Type propagation algorithm not settling */

int FUN_0602a3dc(short *param_1)

{
  short sVar1;
  int iVar2;
  short *psVar3;
  undefined1 uVar4;
  ulonglong uVar5;
  
  while( true ) {
    while( true ) {
      psVar3 = DAT_0602acf0;
      if (*param_1 == 0) {
        return *DAT_0602ad2c;
      }
      while( true ) {
        sVar1 = *param_1;
        if (sVar1 != 0) {
          param_1 = param_1 + 1;
        }
        if ((sVar1 == 0 || sVar1 == 0x2f) || sVar1 == 0x5c) break;
        *psVar3 = sVar1;
        psVar3 = psVar3 + 1;
      }
      *psVar3 = 0;
      uVar4 = psVar3[-1] == 0x3a;
      if (!(bool)uVar4) break;
      FUN_06028844(DAT_0602acf4);
      FUN_060288d4();
      if (!(bool)uVar4) {
        return 0;
      }
      *DAT_0602acfc = *DAT_0602acf8;
      FUN_0602a4cc(DAT_0602ad00,DAT_0602ad04);
      FUN_0602a4cc(DAT_0602ad08,DAT_0602ad0c);
    }
    uVar5 = FUN_060291d8(*DAT_0602ad10,DAT_0602ad14);
    iVar2 = (int)uVar5;
    if (iVar2 == 0) {
      return 0;
    }
    if ((uVar5 & 0x1000000000) == 0) break;
    *DAT_0602ad18 = iVar2;
    FUN_0602a4b8(DAT_0602ad1c,DAT_0602ad20);
    FUN_0602a4b8(DAT_0602ad24,DAT_0602ad28);
  }
  return iVar2;
}

