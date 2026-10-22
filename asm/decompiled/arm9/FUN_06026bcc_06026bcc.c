// Function: FUN_06026bcc
// Address:  06026bcc
// Signature: undefined FUN_06026bcc()


undefined8 FUN_06026bcc(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int iVar8;
  undefined8 uVar9;
  undefined4 uStack_48;
  undefined4 uStack_44;
  undefined4 *puStack_40;
  undefined4 *puStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  
  uStack_38 = param_1;
  uStack_34 = param_2;
  uStack_30 = param_3;
  uStack_2c = param_4;
  uVar9 = FUN_06026b38(param_1,DAT_06027444);
  FUN_06026b38((int)uVar9,(int)((ulonglong)uVar9 >> 0x20) + -4);
  iVar8 = *DAT_06027448;
  puVar4 = DAT_0602744c + 0x12;
  puVar7 = (undefined4 *)((int)DAT_06027450 + iVar8);
  puVar2 = DAT_0602744c;
  puVar5 = DAT_06027450;
  do {
    puVar6 = puVar5 + 1;
    uVar1 = FUN_0602bf6c(*puVar5);
    puVar3 = puVar2 + 1;
    *puVar2 = *puVar2 ^ uVar1;
    if (puVar6 == puVar7) {
      puVar6 = (undefined4 *)((int)puVar6 - iVar8);
    }
    puVar2 = puVar3;
    puVar5 = puVar6;
  } while (puVar3 != puVar4);
  uStack_48 = 0;
  uStack_44 = 0;
  puStack_40 = DAT_06027454;
  puStack_3c = DAT_06027458;
  do {
    FUN_06026b38(uStack_48,&uStack_48);
    puVar5 = puStack_40 + 1;
    *puStack_40 = uStack_44;
    puStack_40 = puStack_40 + 2;
    *puVar5 = uStack_48;
  } while (puStack_40 != puStack_3c);
  return CONCAT44(uStack_34,uStack_38);
}

