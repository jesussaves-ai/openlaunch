// Function: FUN_06029e10
// Address:  06029e10
// Signature: undefined FUN_06029e10()


void FUN_06029e10(uint param_1,char *param_2)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = 8;
  do {
    uVar2 = param_1 >> 0x1c;
    param_1 = uVar2 | param_1 << 4;
    if (9 < uVar2) {
      uVar2 = uVar2 + 7;
    }
    *param_2 = (char)uVar2 + '0';
    iVar1 = iVar1 + -1;
    param_2 = param_2 + 1;
  } while (iVar1 != 0);
  return;
}

