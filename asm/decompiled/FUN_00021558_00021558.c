// Function: FUN_00021558
// Address: 00021558


void FUN_00021558(int param_1)

{
  undefined1 uVar1;
  undefined2 uVar2;
  undefined1 *puVar3;
  undefined1 *extraout_r1;
  undefined1 *extraout_r1_00;
  undefined1 *extraout_r1_01;
  undefined1 *puVar4;
  undefined1 *extraout_r1_02;
  undefined1 *extraout_r1_03;
  undefined1 *extraout_r1_04;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  
  puVar3 = (undefined1 *)FUN_000248c8(0x224);
  if (puVar3 != (undefined1 *)0x0) {
    FUN_0001c72c(puVar3,0x224);
    iVar6 = *DAT_00021c4c;
    *puVar3 = 1;
    puVar3[1] = 1;
    puVar3[2] = 6;
    *(undefined4 *)(puVar3 + 4) = *DAT_00021c50;
    FUN_0001cf0c(*DAT_00021c54 - *DAT_00021c58,1000);
    uVar2 = FUN_00022078();
    *(undefined2 *)(puVar3 + 8) = uVar2;
    if (param_1 == 7) {
      *(undefined4 *)(puVar3 + 0xc) = *(undefined4 *)(iVar6 + 0x10);
    }
    FUN_00024aa4(iVar6 + 0x2c,puVar3 + 0x1c);
    FUN_0001c92c(DAT_00021c5c,puVar3 + 0xec);
    *extraout_r1 = 0x35;
    extraout_r1[1] = 1;
    extraout_r1[2] = (char)param_1;
    extraout_r1[3] = 0x3d;
    extraout_r1[4] = 7;
    extraout_r1[5] = 1;
    FUN_00024aa4(iVar6 + 0x2c,extraout_r1 + 6);
    *extraout_r1_00 = 0xc;
    uVar7 = DAT_00021c60;
    uVar1 = FUN_0001ca50();
    extraout_r1_00[1] = uVar1;
    FUN_0001c7d0(uVar7,extraout_r1_00 + 2);
    *extraout_r1_01 = 0x37;
    uVar5 = *DAT_00021c64;
    extraout_r1_01[2] = 1;
    extraout_r1_01[3] = 3;
    puVar4 = extraout_r1_01 + 4;
    if ((uVar5 & 1) != 0) {
      puVar4 = extraout_r1_01 + 5;
      extraout_r1_01[4] = 6;
    }
    extraout_r1_01[1] = (char)puVar4 - (char)(extraout_r1_01 + 2);
    if (param_1 == 3) {
      *puVar4 = 0x32;
      puVar4[1] = 4;
      FUN_0001c92c(*DAT_00021c68,puVar4 + 2);
      *extraout_r1_02 = 0x36;
      extraout_r1_02[1] = 4;
      FUN_0001c92c(*DAT_00021c6c,extraout_r1_02 + 2);
      puVar4 = extraout_r1_03;
    }
    *puVar4 = 0x3c;
    uVar7 = DAT_00021c70;
    uVar1 = FUN_0001ca50();
    puVar4[1] = uVar1;
    FUN_0001c7d0(uVar7,puVar4 + 2);
    *extraout_r1_04 = 0xff;
    puVar4 = extraout_r1_04 + (1 - (int)puVar3);
    if (puVar4 < (undefined1 *)0x12c) {
      puVar4 = (undefined1 *)0x12c;
    }
    FUN_00022c80(*DAT_00021c74,puVar3,puVar4,0,param_1);
    *DAT_00021c7c = *DAT_00021c78;
    FUN_000249f8(puVar3);
  }
  return;
}

