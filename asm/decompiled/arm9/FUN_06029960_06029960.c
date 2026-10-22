// Function: FUN_06029960
// Address:  06029960
// Signature: undefined FUN_06029960()


void FUN_06029960(void)

{
  int iVar1;
  undefined4 uVar2;
  int extraout_r1;
  int extraout_r1_00;
  int extraout_r1_01;
  int extraout_r1_02;
  int iVar3;
  undefined1 uVar4;
  
  iVar1 = DAT_0602a24c;
  uVar4 = (*(byte *)(DAT_0602a24c + 0x12) & 2) == 0;
  if (!(bool)uVar4) {
    FUN_06029a30(*(byte *)(DAT_0602a24c + 0x12),DAT_0602a250);
    FUN_0602a4cc(DAT_0602a254,DAT_0602a258);
    FUN_0602a538(DAT_0602a25c,extraout_r1 + -0x12);
    if ((bool)uVar4) {
      FUN_0602a4e0(DAT_0602a260,extraout_r1 + -0x10);
      iVar3 = extraout_r1_00;
    }
    else {
      *(undefined2 *)(extraout_r1 + -2) = 0x2f;
      FUN_0602a4cc(DAT_0602a264);
      FUN_0602a4e0(DAT_0602a268,extraout_r1_01 + -8);
      iVar3 = extraout_r1_02;
    }
    FUN_06029a0c(*(undefined4 *)(iVar1 + 0x238),DAT_0602a26c);
    if (*(short *)(iVar3 + -8) == 0x73) {
      iVar3 = iVar3 + -0xb;
      uVar2 = DAT_0602a270;
    }
    else {
      iVar3 = iVar3 + -4;
      uVar2 = DAT_0602a274;
    }
    FUN_0602a4e0(uVar2,iVar3);
    FUN_06029a0c(*(undefined4 *)(iVar1 + 0x23c),DAT_0602a278);
  }
  return;
}

