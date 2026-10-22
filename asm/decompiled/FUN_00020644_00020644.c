// Function: FUN_00020644
// Address: 00020644


undefined8 FUN_00020644(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 extraout_r1;
  undefined4 extraout_r1_00;
  undefined4 uVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 *unaff_r11;
  undefined4 *in_r12;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined8 uVar7;
  
  uVar2 = unaff_r11[2];
  uVar3 = unaff_r11[3];
  FUN_00020740(*unaff_r11,unaff_r11[1]);
  puVar5 = DAT_00020a9c;
  iVar4 = *DAT_00020aa0 + -1;
  uVar1 = extraout_r1;
  do {
    FUN_00020758();
    *puVar5 = uVar1;
    FUN_00020758();
    puVar5[1] = uVar2;
    uVar1 = FUN_00020758();
    puVar5[2] = uVar3;
    FUN_00020758();
    puVar5[3] = uVar1;
    uVar2 = puVar5[2];
    uVar3 = puVar5[3];
    uVar7 = FUN_00020740(*puVar5,puVar5[1]);
    uVar1 = (undefined4)((ulonglong)uVar7 >> 0x20);
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  FUN_00020788();
  puVar5 = in_r12 + 1;
  *in_r12 = (int)uVar7;
  FUN_00020788();
  puVar6 = puVar5 + 1;
  *puVar5 = extraout_r1_00;
  FUN_00020788();
  puVar5 = puVar6 + 1;
  *puVar6 = uVar2;
  FUN_00020788();
  *puVar5 = uVar3;
  return CONCAT44(param_2,param_1);
}

