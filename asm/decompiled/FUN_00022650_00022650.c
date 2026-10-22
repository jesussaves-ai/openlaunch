// Function: FUN_00022650
// Address: 00022650


int FUN_00022650(int *param_1,undefined4 param_2,uint param_3,uint param_4)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = 0;
  if (0 < (int)param_4) {
    if (*param_1 - param_3 < param_4) {
      param_4 = *param_1 - param_3;
    }
    for (; param_1 != (int *)0x0; param_1 = (int *)param_1[2]) {
      if (param_3 < (uint)param_1[1]) {
        do {
          uVar1 = param_1[1];
          if (param_4 < (uint)param_1[1]) {
            uVar1 = param_4;
          }
          param_4 = param_4 - uVar1;
          iVar2 = iVar2 + uVar1;
          FUN_0001c7d0(param_2,param_1[3] + param_3);
          param_3 = 0;
          if (param_4 != 0) {
            param_1 = (int *)param_1[2];
          }
        } while (param_4 != 0 && param_1 != (int *)0x0);
        return iVar2;
      }
      param_3 = param_3 - param_1[1];
    }
  }
  return 0;
}

