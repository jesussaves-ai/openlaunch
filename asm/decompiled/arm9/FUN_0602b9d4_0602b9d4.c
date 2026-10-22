// Function: FUN_0602b9d4
// Address:  0602b9d4
// Signature: undefined FUN_0602b9d4()


void FUN_0602b9d4(undefined4 param_1,undefined4 param_2,int param_3,int param_4)

{
  int iVar1;
  undefined4 uVar2;
  int unaff_r4;
  undefined4 uVar3;
  
  uVar3 = *(undefined4 *)(unaff_r4 + 0x1a4);
  *(uint *)(unaff_r4 + 0x1a4) = *(uint *)(DAT_0602bf0c + 100) | DAT_0602bf10;
  if (param_3 != 0) {
    iVar1 = DAT_0602bf14 + (0x4000 - param_3);
    uVar2 = 0;
    do {
      FUN_0602bce4(iVar1,uVar2,0x1000,param_4,iVar1,uVar2,param_3,param_4);
      iVar1 = iVar1 + 0x1000;
      param_4 = param_4 + 0x1000;
      param_3 = param_3 + -0x1000;
    } while (param_3 != 0);
  }
  *(undefined4 *)(unaff_r4 + 0x1a4) = uVar3;
  return;
}

