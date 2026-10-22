// Function: FUN_00029984
// Address: 00029984


undefined8 FUN_00029984(uint param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  
  iVar2 = 4;
  uVar1 = param_1;
  do {
    FUN_00029914(uVar1 & 0xff);
    if (iVar2 != 1) {
      FUN_000296b4(0x2e);
    }
    uVar1 = uVar1 >> 8 | uVar1 << 0x18;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return CONCAT44(param_2,param_1);
}

