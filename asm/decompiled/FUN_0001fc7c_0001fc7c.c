// Function: FUN_0001fc7c
// Address: 0001fc7c


void FUN_0001fc7c(undefined4 *param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  
  uVar1 = DAT_000205f8[1];
  uVar2 = DAT_000205f8[2];
  uVar3 = DAT_000205f8[3];
  uVar4 = DAT_000205f8[4];
  uVar5 = DAT_000205f8[5];
  uVar6 = DAT_000205f8[6];
  *param_1 = *DAT_000205f8;
  param_1[1] = uVar1;
  param_1[2] = uVar2;
  param_1[3] = uVar3;
  param_1[4] = uVar4;
  param_1[5] = uVar5;
  param_1[6] = uVar6;
  param_1[0x17] = uVar6;
  uVar1 = DAT_00020600;
  if (*DAT_000205fc != '\0') {
    uVar1 = DAT_00020604;
  }
  param_1[0x18] = uVar1;
  return;
}

