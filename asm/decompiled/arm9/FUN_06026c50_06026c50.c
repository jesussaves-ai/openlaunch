// Function: FUN_06026c50
// Address:  06026c50
// Signature: undefined FUN_06026c50()


undefined8 FUN_06026c50(uint param_1,uint param_2,uint param_3)

{
  uint *puVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  
  puVar1 = DAT_0602745c;
  uVar5 = param_3 >> 7;
  param_3 = param_3 & 0x7f;
  uVar6 = *DAT_0602745c;
  uVar7 = DAT_0602745c[1];
  uVar8 = DAT_0602745c[2];
  uVar9 = DAT_0602745c[3];
  *DAT_0602745c = param_1;
  puVar1[1] = param_2;
  puVar1[2] = param_3;
  puVar1[3] = uVar5;
  if ((uVar6 == param_1 && uVar7 == param_2) && uVar9 == uVar5) {
    if (uVar8 == param_3) goto LAB_06026d04;
    if (param_3 <= uVar8) goto LAB_06026c98;
    if (uVar8 == 1) goto LAB_06026ce0;
  }
  else {
LAB_06026c98:
    puVar1[4] = param_1;
    puVar1[5] = param_1 >> 1;
    puVar1[6] = param_1 << 1;
    if (*DAT_06027460 != DAT_06027464) {
      FUN_060264a0(*DAT_06027460,DAT_06027468);
      do {
                    /* WARNING: Do nothing block with infinite loop */
      } while( true );
    }
    uVar3 = DAT_0602746c;
    if (uVar5 != 0) {
      uVar3 = DAT_06027470;
    }
    FUN_0602c1a8(uVar3,DAT_06027474,DAT_06027478);
    FUN_06026bcc();
LAB_06026ce0:
    if (1 < param_3) {
      FUN_06026bcc();
    }
  }
  puVar2 = DAT_0602747c;
  iVar4 = DAT_0602747c[1];
  uVar5 = DAT_0602747c[2];
  if (2 < param_3) {
    iVar4 = iVar4 << 1;
    uVar5 = uVar5 >> 1;
  }
  *DAT_0602747c = *DAT_0602747c;
  puVar2[1] = iVar4;
  puVar2[2] = uVar5;
  if (2 < param_3) {
    FUN_06026bcc();
  }
LAB_06026d04:
  return CONCAT44(param_2,param_1);
}

