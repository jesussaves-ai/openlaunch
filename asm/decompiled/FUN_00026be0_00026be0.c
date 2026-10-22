// Function: FUN_00026be0
// Address: 00026be0


void FUN_00026be0(undefined4 param_1,int param_2,int param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  undefined8 uVar5;
  
  uVar1 = param_3 + param_2 + DAT_00026e58;
  uVar4 = DAT_00026e58 + 8000;
  while( true ) {
    if (uVar4 <= uVar1) {
      uVar1 = uVar1 - 0x1340;
    }
    if (param_4 == 0) break;
    uVar3 = uVar4 - uVar1;
    uVar2 = param_4;
    if (uVar3 < param_4) {
      uVar2 = uVar3;
    }
    param_4 = param_4 - uVar2;
    uVar5 = FUN_0001c7d0(uVar1,param_1);
    param_1 = (undefined4)((ulonglong)uVar5 >> 0x20);
    uVar1 = (uint)uVar5;
  }
  return;
}

