// Function: FUN_0602bce4
// Address:  0602bce4
// Signature: undefined FUN_0602bce4()


void FUN_0602bce4(int param_1,undefined4 param_2,uint param_3,int param_4)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int unaff_r4;
  bool bVar6;
  uint uVar7;
  
  iVar3 = *DAT_0602bf40;
  *DAT_0602bf40 = iVar3 + 1;
  piVar1 = DAT_0602bf44;
  *DAT_0602bf44 = iVar3;
  piVar1[1] = param_1;
  uVar7 = param_3;
  iVar3 = param_4;
  FUN_06026b38(param_1,piVar1);
  iVar4 = *piVar1;
  iVar2 = piVar1[1];
  if ((*DAT_0602bf48 & 0x80000000) == 0) {
    FUN_0602bd94();
  }
  else {
    *(uint *)(unaff_r4 + 0x1a4) = *(uint *)(unaff_r4 + 0x1a4) & 0xefffffff;
    FUN_0602bd94(iVar2,iVar4,0,param_4,iVar2,iVar4,param_3,param_4,uVar7,iVar3);
    software_interrupt(0x30000);
    do {
      uVar5 = param_3;
      if (0x200 < param_3) {
        uVar5 = 0x200;
      }
      FUN_0602bd94(iVar2,iVar4,uVar5,param_4,iVar2,iVar4,param_3,param_4,uVar7,iVar3);
      param_4 = param_4 + 0x200;
      bVar6 = 0x1ff < param_3;
      param_3 = param_3 - 0x200;
    } while (bVar6 && param_3 != 0);
  }
  return;
}

