// Function: FUN_00022504
// Address: 00022504


uint FUN_00022504(int *param_1,uint param_2,uint param_3)

{
  ushort *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  
  uVar4 = 0;
  if (0 < (int)param_3) {
    if (*param_1 - param_2 < param_3) {
      param_3 = *param_1 - param_2;
    }
    for (; param_1 != (int *)0x0; param_1 = (int *)param_1[2]) {
      if (param_2 < (uint)param_1[1]) {
        uVar3 = 0;
        do {
          puVar1 = (ushort *)(param_1[3] + param_2);
          uVar2 = param_1[1];
          if (param_3 < (uint)param_1[1]) {
            uVar2 = param_3;
          }
          param_3 = param_3 - uVar2;
          if ((((uint)puVar1 & 1) == 0 && (uVar2 & 1) == 0) && uVar3 == 0) {
            do {
              uVar4 = uVar4 + *puVar1;
              uVar2 = uVar2 - 2;
              puVar1 = puVar1 + 1;
            } while (uVar2 != 0);
          }
          else {
            do {
              uVar4 = uVar4 + ((uint)(byte)*puVar1 << uVar3);
              uVar3 = uVar3 ^ 8;
              uVar2 = uVar2 - 1;
              puVar1 = (ushort *)((int)puVar1 + 1);
            } while (uVar2 != 0);
          }
          bVar5 = CARRY4(uVar4,uVar4 * 0x10000);
          uVar2 = uVar4 * 0x10001;
          uVar4 = uVar2;
          if (bVar5) {
            uVar4 = uVar2 + 0x10000;
          }
          if (bVar5 && 0xfffeffff < uVar2) {
            uVar4 = uVar4 + 0x10000;
          }
          uVar4 = uVar4 >> 0x10;
          param_2 = 0;
          if (param_3 != 0) {
            param_1 = (int *)param_1[2];
          }
        } while (param_3 != 0 && param_1 != (int *)0x0);
        return uVar4;
      }
      param_2 = param_2 - param_1[1];
    }
  }
  return 0;
}

