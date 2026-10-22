// Function: FUN_060265fc
// Address:  060265fc
// Signature: undefined FUN_060265fc()


undefined8 FUN_060265fc(uint param_1,undefined4 param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  
  uVar1 = DAT_06026794;
  uVar2 = 0;
  uVar3 = param_1;
  do {
    software_interrupt(0x90000);
    uVar2 = uVar2 | uVar3;
    if (uVar2 != 0 || (uVar1 & 1) != 0) {
      FUN_06026324(uVar3 | 0x30);
    }
    software_interrupt(0x90000);
    uVar3 = uVar1;
  } while (uVar1 != 0);
  return CONCAT44(param_2,param_1);
}

