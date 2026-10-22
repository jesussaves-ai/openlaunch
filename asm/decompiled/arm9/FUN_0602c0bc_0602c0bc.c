// Function: FUN_0602c0bc
// Address:  0602c0bc
// Signature: undefined FUN_0602c0bc()


ulonglong FUN_0602c0bc(undefined4 *param_1,uint param_2)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  bool bVar4;
  
  iVar1 = DAT_0602c4d0;
  if (param_2 == 0) {
    return ZEXT48(param_1);
  }
  if (((uint)param_1 & 3) != 0 || (param_2 & 3) != 0) {
    if (param_2 != 0) {
      puVar2 = param_1;
      if (((uint)param_1 & 1) != 0) {
        puVar2 = (undefined4 *)((int)param_1 + 1);
        *(undefined1 *)param_1 = 0;
        param_2 = param_2 - 1;
      }
      puVar3 = puVar2;
      if (1 < param_2) {
        if (((uint)puVar2 & 2) != 0) {
          puVar3 = (undefined4 *)((int)puVar2 + 2);
          *(undefined2 *)puVar2 = 0;
          param_2 = param_2 - 2;
        }
        if (0x27 < param_2) {
          while (0x27 < param_2) {
            *puVar3 = 0;
            puVar3[1] = 0;
            puVar3[2] = 0;
            puVar3[3] = 0;
            puVar3[4] = 0;
            puVar3[5] = 0;
            puVar3[6] = 0;
            puVar3[7] = 0;
            puVar3[8] = 0;
            puVar3[9] = 0;
            puVar3 = puVar3 + 10;
            param_2 = param_2 - 0x28;
          }
        }
        while (3 < param_2) {
          *puVar3 = 0;
          puVar3 = puVar3 + 1;
          param_2 = param_2 - 4;
        }
        while (1 < param_2) {
          *(undefined2 *)puVar3 = 0;
          puVar3 = (undefined4 *)((int)puVar3 + 2);
          param_2 = param_2 - 2;
        }
      }
      while (bVar4 = param_2 != 0, param_2 = param_2 - 1, bVar4) {
        *(undefined1 *)puVar3 = 0;
        puVar3 = (undefined4 *)((int)puVar3 + 1);
      }
      return ZEXT48(puVar3);
    }
    return ZEXT48(param_1);
  }
  *(undefined4 *)(DAT_0602c4d0 + 0x68) = 0;
  *(undefined4 *)(iVar1 + 0x6c) = 0;
  *(undefined4 *)(iVar1 + 0x58) = 0;
  *(undefined4 **)(iVar1 + 0x5c) = param_1;
  *(uint *)(iVar1 + 0x60) = param_2 >> 2;
  *(uint *)(iVar1 + 100) = param_2 >> 2;
  *(undefined4 *)(iVar1 + 0x70) = DAT_0602c4d4;
  return CONCAT44(param_2,(undefined1 *)((int)param_1 + param_2));
}

