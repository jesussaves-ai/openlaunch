// Function: FUN_000292c4
// Address: 000292c4


uint FUN_000292c4(undefined4 param_1,int param_2,uint param_3)

{
  undefined4 *puVar1;
  uint uVar2;
  uint uVar3;
  undefined4 uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  undefined4 uVar8;
  
  iVar7 = 0;
  uVar2 = FUN_0001cf0c(param_3,100);
  if (uVar2 < 0x400) {
    uVar2 = 0x400;
  }
  uVar6 = param_3;
  if (param_3 != 0) {
    while( true ) {
      puVar1 = DAT_00029bd4;
      uVar8 = *DAT_00029bd4;
      FUN_0001cf0c(iVar7 * 100,param_3);
      FUN_00029914();
      FUN_000296b4(0x25);
      FUN_00029840();
      uVar3 = FUN_0001cf0c(*DAT_00029bd8 * 0x3c,(*DAT_00029bdc - *DAT_00029be0) + 1);
      uVar4 = FUN_00029914(uVar3 >> 10);
      FUN_00029878(uVar4,DAT_00029be4);
      FUN_000291ac();
      *puVar1 = uVar8;
      if (uVar6 == 0) break;
      uVar3 = uVar2;
      if (uVar6 < uVar2) {
        uVar3 = uVar6;
      }
      iVar5 = FUN_00029248(param_1,param_2,uVar3,0);
      if (iVar5 == 0) {
        FUN_000296b4(0x2d);
        FUN_000296b4(0x45);
        FUN_000296b4(0x52);
        FUN_000296b4(0x52);
        FUN_000296b4(0x2d);
        return uVar6;
      }
      uVar6 = uVar6 - iVar5;
      param_2 = param_2 + iVar5;
      iVar7 = iVar7 + iVar5;
      *DAT_00029be8 = *DAT_00029be8 + iVar5;
    }
  }
  return 0;
}

