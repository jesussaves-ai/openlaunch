// Function: FUN_06025518
// Address:  06025518
// Signature: undefined FUN_06025518()


undefined8 FUN_06025518(undefined4 param_1,undefined4 param_2)

{
  undefined1 *puVar1;
  undefined1 in_ZR;
  undefined8 uVar2;
  undefined8 uVar3;
  
  uVar2 = FUN_060254f8();
  do {
    uVar3 = FUN_06025484((int)uVar2,(int)((ulonglong)uVar2 >> 0x20));
    puVar1 = (undefined1 *)((ulonglong)uVar3 >> 0x20);
    uVar2 = CONCAT44(puVar1 + 1,(int)uVar3);
    *puVar1 = (char)uVar3;
  } while (!(bool)in_ZR);
  return CONCAT44(param_2,param_1);
}

