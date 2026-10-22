// Function: FUN_00022d08
// Address: 00022d08


int FUN_00022d08(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  int unaff_r4;
  undefined4 *unaff_r5;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 == (uint *)0x0 || unaff_r4 == 0) || ((*param_1 & 0x2000) == 0)) ||
     ((*param_1 & 1) != 0)) {
    iVar2 = FUN_0001cd28();
  }
  else {
    *unaff_r5 = 0x10;
    while (((iVar2 = FUN_00024728(param_1[1],param_2,param_3,param_4), iVar2 == -1 &&
            (*DAT_0002307c == 2)) && ((*param_1 & 0x4000) == 0))) {
      FUN_00020af8(uVar1);
      FUN_0001cad0();
      FUN_00020ae4();
    }
  }
  FUN_00020af8(uVar1);
  return iVar2;
}

