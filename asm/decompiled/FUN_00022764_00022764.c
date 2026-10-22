// Function: FUN_00022764
// Address: 00022764


uint * FUN_00022764(int param_1,int param_2,int param_3)

{
  undefined4 uVar1;
  uint uVar2;
  uint *puVar3;
  int iVar4;
  undefined *puVar5;
  
  uVar1 = FUN_00020ae4();
  if (((param_2 == 2 || param_2 == 1) && param_1 == 2) && param_3 == 0) {
    iVar4 = 0x20;
    puVar3 = DAT_00023060;
    do {
      uVar2 = *puVar3;
      if ((uVar2 & 0x8000) == 0) {
        puVar5 = (undefined *)0xa000;
        if (param_2 == 1) {
          puVar5 = &DAT_0000a001;
          uVar2 = FUN_0002342c();
        }
        if (param_2 == 2) {
          uVar2 = FUN_0002462c();
        }
        if (uVar2 != 0) {
          puVar3[1] = uVar2;
          *puVar3 = (uint)puVar5;
          goto LAB_000227f0;
        }
        break;
      }
      puVar3 = puVar3 + 2;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    puVar3 = (uint *)FUN_0001cd38();
  }
  else {
    puVar3 = (uint *)FUN_0001cd28();
  }
LAB_000227f0:
  FUN_00020af8(uVar1);
  return puVar3;
}

