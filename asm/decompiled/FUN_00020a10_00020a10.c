// Function: FUN_00020a10
// Address: 00020a10


void FUN_00020a10(undefined4 param_1,undefined4 param_2,undefined4 *param_3,int param_4)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 *puVar6;
  int iVar8;
  undefined4 *puVar7;
  
  FUN_000208b4(param_1,0x80);
  FUN_0001c7d0(param_2,param_3,param_4);
  iVar5 = (param_4 - 8U >> 2) + (param_4 - 8U >> 1);
  do {
    iVar8 = param_4 + -8;
    puVar6 = (undefined4 *)((int)param_3 + param_4);
    do {
      puVar7 = puVar6 + -2;
      uVar1 = *param_3;
      uVar2 = param_3[1];
      uVar3 = *puVar7;
      uVar4 = puVar6[-1];
      FUN_00020644();
      *param_3 = uVar1;
      param_3[1] = uVar2 ^ iVar5 << 0x18;
      *puVar7 = uVar3;
      puVar6[-1] = uVar4;
      iVar5 = iVar5 + -1;
      iVar8 = iVar8 + -8;
      puVar6 = puVar7;
    } while (iVar8 != 0);
  } while (iVar5 != 0);
  return;
}

