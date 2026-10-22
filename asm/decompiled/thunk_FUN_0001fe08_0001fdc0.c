// Function: thunk_FUN_0001fe08
// Address: 0001fdc0


int thunk_FUN_0001fe08(undefined4 *param_1,undefined4 *param_2)

{
  undefined1 uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined1 *puVar4;
  uint uVar6;
  uint uVar7;
  int iVar8;
  undefined1 auStack_58 [72];
  undefined1 *puVar5;
  
  if (param_2[0x18] == DAT_00020608) {
    iVar8 = 0x10;
  }
  else {
    iVar8 = 0x14;
  }
  uVar6 = param_2[0x17];
  uVar1 = 0x80;
  puVar5 = auStack_58;
  do {
    puVar4 = puVar5 + 1;
    *puVar5 = uVar1;
    uVar1 = 0;
    uVar6 = uVar6 + 1 & 0x3f;
    puVar5 = puVar4;
  } while (uVar6 != 0x38);
  uVar6 = param_2[5];
  uVar7 = param_2[6];
  iVar2 = 0x40;
  do {
    if (iVar8 == 0x14) {
      *puVar4 = (char)(uVar7 >> 0x18);
      uVar7 = uVar7 << 8 | uVar6 >> 0x18;
      uVar6 = uVar6 << 8;
    }
    else {
      *puVar4 = (char)uVar6;
      uVar6 = uVar6 >> 8 | uVar7 << 0x18;
      uVar7 = uVar7 >> 8;
    }
    puVar4 = puVar4 + 1;
    iVar2 = iVar2 + -8;
  } while (iVar2 != 0);
  FUN_0001fcd4(param_2,auStack_58,(int)puVar4 - (int)auStack_58);
  iVar2 = iVar8;
  do {
    uVar3 = *param_2;
    if (iVar8 == 0x14) {
      uVar3 = FUN_0001cd8c();
    }
    *param_1 = uVar3;
    iVar2 = iVar2 + -4;
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
  } while (iVar2 != 0);
  return iVar8;
}

