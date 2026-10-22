// Function: FUN_06027d60
// Address:  06027d60
// Signature: undefined FUN_06027d60()


void FUN_06027d60(int param_1,int param_2,uint param_3)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  
  do {
    uVar3 = param_3;
    if (500 < param_3) {
      uVar3 = 500;
    }
    FUN_0602c1a8(param_1,DAT_06028374,uVar3);
    FUN_06027cb8();
    puVar1 = DAT_06028378;
    iVar2 = DAT_0602837c - (uVar3 + 0xc);
    *DAT_06028378 = 3;
    puVar1[1] = param_2;
    puVar1[2] = uVar3;
    FUN_06028018(puVar1,iVar2);
    param_1 = param_1 + uVar3;
    param_2 = param_2 + uVar3;
    param_3 = param_3 - uVar3;
  } while (param_3 != 0);
  return;
}

