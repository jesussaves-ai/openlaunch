// Function: FUN_0001c72c
// Address: 0001c72c


void FUN_0001c72c(undefined4 *param_1,uint param_2)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  bool bVar3;
  
  if (param_2 == 0) {
    return;
  }
  puVar1 = param_1;
  if (((uint)param_1 & 1) != 0) {
    puVar1 = (undefined4 *)((int)param_1 + 1);
    *(undefined1 *)param_1 = 0;
    param_2 = param_2 - 1;
  }
  puVar2 = puVar1;
  if (1 < param_2) {
    if (((uint)puVar1 & 2) != 0) {
      puVar2 = (undefined4 *)((int)puVar1 + 2);
      *(undefined2 *)puVar1 = 0;
      param_2 = param_2 - 2;
    }
    if (0x27 < param_2) {
      while (0x27 < param_2) {
        *puVar2 = 0;
        puVar2[1] = 0;
        puVar2[2] = 0;
        puVar2[3] = 0;
        puVar2[4] = 0;
        puVar2[5] = 0;
        puVar2[6] = 0;
        puVar2[7] = 0;
        puVar2[8] = 0;
        puVar2[9] = 0;
        puVar2 = puVar2 + 10;
        param_2 = param_2 - 0x28;
      }
    }
    while (3 < param_2) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
      param_2 = param_2 - 4;
    }
    while (1 < param_2) {
      *(undefined2 *)puVar2 = 0;
      puVar2 = (undefined4 *)((int)puVar2 + 2);
      param_2 = param_2 - 2;
    }
  }
  while (bVar3 = param_2 != 0, param_2 = param_2 - 1, bVar3) {
    *(undefined1 *)puVar2 = 0;
    puVar2 = (undefined4 *)((int)puVar2 + 1);
  }
  return;
}

