// Function: FUN_0001cfb0
// Address: 0001cfb0


undefined8 FUN_0001cfb0(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined1 *puVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  
  iVar1 = FUN_0001cf1c();
  uVar3 = FUN_0001cf54(iVar1 << 4 | 0x86);
  do {
    uVar4 = FUN_0001cf60((int)uVar3,(int)((ulonglong)uVar3 >> 0x20));
    puVar2 = (undefined1 *)((ulonglong)uVar4 >> 0x20);
    uVar3 = CONCAT44(puVar2 + 1,(int)uVar4);
    *puVar2 = (char)uVar4;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  FUN_0001cf40();
  return CONCAT44(param_2,param_1);
}

