// Function: FUN_0001d6e4
// Address: 0001d6e4


void FUN_0001d6e4(int param_1,int param_2,uint param_3)

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
    FUN_0001d6b8();
    uVar2 = DAT_0001dc58;
    puVar1 = DAT_0001dc54;
    *DAT_0001dc54 = 2;
    puVar1[1] = param_1;
    puVar1[2] = uVar3;
    uVar4 = FUN_0001d958(puVar1,uVar2,0xc);
    FUN_0001d750((int)uVar4,(int)((ulonglong)uVar4 >> 0x20),uVar3);
    FUN_0001c7d0(param_2,extraout_r1,uVar3);
    param_1 = param_1 + uVar3;
    param_2 = param_2 + uVar3;
    param_3 = param_3 - uVar3;
  } while (param_3 != 0);
  return;
}

