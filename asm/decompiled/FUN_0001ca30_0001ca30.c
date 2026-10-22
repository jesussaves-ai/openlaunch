// Function: FUN_0001ca30
// Address: 0001ca30


undefined8 FUN_0001ca30(undefined4 param_1,undefined4 param_2)

{
  undefined1 *puVar1;
  undefined1 in_ZR;
  undefined8 uVar2;
  undefined8 uVar3;
  
  uVar2 = FUN_0001ca10();
  do {
    uVar3 = FUN_0001c984((int)uVar2,(int)((ulonglong)uVar2 >> 0x20));
    puVar1 = (undefined1 *)((ulonglong)uVar3 >> 0x20);
    uVar2 = CONCAT44(puVar1 + 1,(int)uVar3);
    *puVar1 = (char)uVar3;
  } while (!(bool)in_ZR);
  return CONCAT44(param_2,param_1);
}

