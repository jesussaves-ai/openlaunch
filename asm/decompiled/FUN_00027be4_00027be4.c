// Function: FUN_00027be4
// Address: 00027be4


void FUN_00027be4(void)

{
  byte bVar1;
  undefined2 uVar2;
  undefined1 *puVar3;
  undefined2 *extraout_r1;
  undefined1 *extraout_r1_00;
  int extraout_r1_01;
  char cVar4;
  int iVar5;
  uint uVar6;
  byte *pbVar7;
  byte *pbVar8;
  undefined1 auStack_88 [128];
  
  FUN_00027a8c(0,auStack_88);
  if (*DAT_00028278 == '\0') {
    uVar2 = 0x21;
  }
  else {
    uVar2 = 0x31;
  }
  *extraout_r1 = uVar2;
  extraout_r1[1] = *(undefined2 *)(DAT_0002827c + 0x8e);
  *(undefined1 *)(extraout_r1 + 2) = 0;
  puVar3 = DAT_00028280 + 1;
  *(undefined1 *)((int)extraout_r1 + 5) = *DAT_00028280;
  FUN_0001c7d0(puVar3,extraout_r1 + 3);
  pbVar8 = DAT_00028284;
  if ((*DAT_00028284 & 0x7f) != 2) {
    iVar5 = 0xe;
    do {
      uVar6 = iVar5 + 1;
      pbVar8[uVar6] = pbVar8[iVar5];
      iVar5 = iVar5 + -1;
    } while (1 < uVar6);
  }
  *pbVar8 = 0x82;
  if ((pbVar8[1] & 0x7f) != 4) {
    iVar5 = 0xe;
    do {
      uVar6 = iVar5 + 1;
      pbVar8[uVar6] = pbVar8[iVar5];
      iVar5 = iVar5 + -1;
    } while (1 < uVar6 && iVar5 != 0);
  }
  pbVar8[1] = 4;
  pbVar8[0xf] = 0;
  cVar4 = '\0';
  pbVar7 = pbVar8;
  while (*pbVar7 != 0) {
    cVar4 = cVar4 + '\x01';
    pbVar7 = pbVar7 + 1;
  }
  pbVar8 = pbVar8 + 2;
  do {
    bVar1 = *pbVar8;
    *pbVar8 = bVar1 & 0x7f;
    pbVar8 = pbVar8 + 1;
  } while ((bVar1 & 0x7f) != 0);
  *extraout_r1_00 = 1;
  extraout_r1_00[1] = cVar4;
  FUN_0001c7d0(DAT_00028288,extraout_r1_00 + 2);
  FUN_000279fc(auStack_88,extraout_r1_01 - (int)auStack_88);
  return;
}

