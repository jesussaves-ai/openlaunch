// Function: FUN_0001d760
// Address: 0001d760


void FUN_0001d760(int param_1,int param_2,uint param_3)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  
  do {
    uVar3 = param_3;
    if (500 < param_3) {
      uVar3 = 500;
    }
    FUN_0001c7d0(param_1,DAT_0001dc68,uVar3);
    FUN_0001d6b8();
    puVar1 = DAT_0001dc6c;
    iVar2 = DAT_0001dc70 - (uVar3 + 0xc);
    *DAT_0001dc6c = 3;
    puVar1[1] = param_2;
    puVar1[2] = uVar3;
    FUN_0001d958(puVar1,iVar2);
    param_1 = param_1 + uVar3;
    param_2 = param_2 + uVar3;
    param_3 = param_3 - uVar3;
  } while (param_3 != 0);
  return;
}

