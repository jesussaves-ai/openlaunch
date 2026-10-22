// Function: FUN_0602a3c4
// Address:  0602a3c4
// Signature: undefined FUN_0602a3c4()


/* WARNING: Type propagation algorithm not settling */

int FUN_0602a3c4(byte *param_1)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  ushort *puVar4;
  short *psVar5;
  short *psVar6;
  undefined1 uVar7;
  ulonglong uVar8;
  
  puVar4 = DAT_0602ace8;
  do {
    bVar1 = *param_1;
    *puVar4 = (ushort)bVar1;
    param_1 = param_1 + 1;
    puVar4 = puVar4 + 1;
    psVar6 = DAT_0602acec;
  } while (bVar1 != 0);
  while( true ) {
    while( true ) {
      psVar5 = DAT_0602acf0;
      if (*psVar6 == 0) {
        return *DAT_0602ad2c;
      }
      while( true ) {
        sVar2 = *psVar6;
        if (sVar2 != 0) {
          psVar6 = psVar6 + 1;
        }
        if ((sVar2 == 0 || sVar2 == 0x2f) || sVar2 == 0x5c) break;
        *psVar5 = sVar2;
        psVar5 = psVar5 + 1;
      }
      *psVar5 = 0;
      uVar7 = psVar5[-1] == 0x3a;
      if (!(bool)uVar7) break;
      FUN_06028844(DAT_0602acf4);
      FUN_060288d4();
      if (!(bool)uVar7) {
        return 0;
      }
      *DAT_0602acfc = *DAT_0602acf8;
      FUN_0602a4cc(DAT_0602ad00,DAT_0602ad04);
      FUN_0602a4cc(DAT_0602ad08,DAT_0602ad0c);
    }
    uVar8 = FUN_060291d8(*DAT_0602ad10,DAT_0602ad14);
    iVar3 = (int)uVar8;
    if (iVar3 == 0) {
      return 0;
    }
    if ((uVar8 & 0x1000000000) == 0) break;
    *DAT_0602ad18 = iVar3;
    FUN_0602a4b8(DAT_0602ad1c,DAT_0602ad20);
    FUN_0602a4b8(DAT_0602ad24,DAT_0602ad28);
  }
  return iVar3;
}

