// Function: FUN_06027ce4
// Address:  06027ce4
// Signature: undefined FUN_06027ce4()


void FUN_06027ce4(int param_1,int param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 extraout_r1;
  uint uVar3;
  undefined8 uVar4;
  
  do {
    uVar3 = param_3;
    if (0x80 < param_3) {
      uVar3 = 0x80;
    }
    FUN_06027cb8();
    uVar2 = DAT_06028364;
    puVar1 = DAT_06028360;
    *DAT_06028360 = 2;
    puVar1[1] = param_1;
    puVar1[2] = uVar3;
    uVar4 = FUN_06028018(puVar1,uVar2,0xc);
    FUN_06027d50((int)uVar4,(int)((ulonglong)uVar4 >> 0x20),uVar3);
    FUN_0602c1a8(param_2,extraout_r1,uVar3);
    param_1 = param_1 + uVar3;
    param_2 = param_2 + uVar3;
    param_3 = param_3 - uVar3;
  } while (param_3 != 0);
  return;
}

