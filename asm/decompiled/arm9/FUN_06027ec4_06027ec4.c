// Function: FUN_06027ec4
// Address:  06027ec4
// Signature: undefined FUN_06027ec4()


void FUN_06027ec4(int param_1,undefined4 param_2,uint param_3)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  
  do {
    uVar3 = param_3;
    if (0x1f8 < param_3) {
      uVar3 = 0x1f8;
    }
    FUN_0602c1a8(param_1,DAT_060283a4,uVar3);
    FUN_06027cb8();
    puVar1 = DAT_060283a8;
    iVar2 = DAT_060283ac - (uVar3 + 8);
    *DAT_060283a8 = 0xe;
    puVar1[1] = uVar3;
    FUN_06028018(puVar1,iVar2);
    param_1 = param_1 + uVar3;
    param_3 = param_3 - uVar3;
  } while (param_3 != 0);
  return;
}

