// Function: FUN_0001d34c
// Address: 0001d34c


void FUN_0001d34c(void)

{
  undefined4 uVar1;
  uint uVar2;
  undefined8 uVar3;
  
  uVar1 = FUN_0001d82c();
  *DAT_0001db7c = uVar1;
  uVar3 = FUN_0001d628(DAT_0001db80);
  FUN_0001d760((int)uVar3,(int)((ulonglong)uVar3 >> 0x20),4);
  uVar2 = FUN_0001d7c8(DAT_0001db84);
  *DAT_0001db88 = uVar2;
  FUN_0001d7f8(DAT_0001db8c,uVar2 | 8);
  uVar2 = FUN_0001d7c8(DAT_0001db90);
  *DAT_0001db94 = uVar2;
  FUN_0001d7f8(DAT_0001db98,uVar2 | 1);
  FUN_0001d7f8(DAT_0001db9c,5);
  FUN_0001d7f8(DAT_0001dba0,0);
  return;
}

