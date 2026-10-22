// Function: FUN_00022444
// Address: 00022444


void FUN_00022444(uint *param_1,uint param_2)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 *puVar3;
  
  if (param_1 == (uint *)0x0) {
    return;
  }
  *param_1 = param_2;
  uVar2 = 0;
  do {
    uVar2 = uVar2 + param_1[1];
    if (param_2 < uVar2) {
      param_1[1] = param_1[1] - (uVar2 - param_2);
      puVar3 = (undefined4 *)param_1[2];
      param_1[2] = 0;
      if (puVar3 == (undefined4 *)0x0) {
        return;
      }
      uVar1 = FUN_00020ae4();
      while (puVar3 != (undefined4 *)0x0) {
        *puVar3 = 0;
        puVar3[1] = 0;
        puVar3 = (undefined4 *)puVar3[2];
        FUN_000249f8();
      }
      FUN_00020af8(uVar1);
      return;
    }
    param_1 = (uint *)param_1[2];
  } while (param_1 != (uint *)0x0);
  return;
}

