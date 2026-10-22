// Function: FUN_00029914
// Address: 00029914


undefined8 FUN_00029914(undefined4 param_1,undefined4 param_2)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined8 uVar4;
  
  uVar3 = 0;
  uVar2 = param_1;
  uVar1 = DAT_00029c38;
  do {
    uVar4 = FUN_0001cf0c(uVar2,uVar1);
    uVar2 = (undefined4)((ulonglong)uVar4 >> 0x20);
    uVar3 = uVar3 | (uint)uVar4;
    if (uVar3 != 0 || (uVar1 & 1) != 0) {
      FUN_000296b4((uint)uVar4 | 0x30);
    }
    uVar1 = FUN_0001cf0c(uVar1,10);
  } while (uVar1 != 0);
  return CONCAT44(param_2,param_1);
}

