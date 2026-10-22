// Function: FUN_00024d98
// Address: 00024d98


uint FUN_00024d98(int param_1,uint param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  bool bVar8;
  undefined1 auStack_84 [33];
  byte local_63;
  undefined1 auStack_5c [20];
  char local_48;
  
  uVar4 = 0xffffffff;
  iVar1 = FUN_00024cb8();
  iVar5 = DAT_00025240;
  for (; iVar1 != 0; iVar1 = iVar1 + -1) {
    FUN_00024d20(iVar5,auStack_84);
    uVar6 = param_2;
    iVar7 = param_1;
    do {
      if ((((*(byte *)(uVar6 + 0x21) == 0) ||
           (((uVar3 = (uint)local_63, uVar3 == *(byte *)(uVar6 + 0x21) && (uVar3 < 0x21)) &&
            (FUN_0001caa8(uVar6,auStack_84), uVar3 == 0)))) &&
          ((iVar2 = FUN_00024ac0(DAT_00025244,uVar6 + 0x28), iVar2 != 0 ||
           (iVar2 = FUN_00024ac0(auStack_5c,uVar6 + 0x28), iVar2 != 0)))) &&
         ((*(char *)(uVar6 + 0x3c) == '\0' || (*(char *)(uVar6 + 0x3c) == local_48)))) {
        bVar8 = true;
        FUN_00024ea0(auStack_84,uVar6);
        if ((!bVar8) && (uVar6 < uVar4)) {
          FUN_0001c7d0(auStack_84,param_3,0x60);
          *DAT_00025248 = iVar5;
          uVar4 = uVar6;
        }
      }
      uVar6 = uVar6 + 0xac;
      iVar7 = iVar7 + -1;
    } while (iVar7 != 0);
    iVar5 = iVar5 + 0x60;
  }
  return uVar4;
}

