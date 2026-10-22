// Function: FUN_000298f8
// Address: 000298f8


void FUN_000298f8(uint param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined8 uVar2;
  
  iVar1 = 8;
  do {
    uVar2 = FUN_000298ac(param_1 >> 0x1c | param_1 << 4,iVar1,param_3,param_4,param_2);
    param_1 = (uint)uVar2;
    iVar1 = (int)((ulonglong)uVar2 >> 0x20) + -1;
  } while (iVar1 != 0);
  return;
}

