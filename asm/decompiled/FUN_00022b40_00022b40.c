// Function: FUN_00022b40
// Address: 00022b40


int FUN_00022b40(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 == (uint *)0x0) || ((*param_1 & 0x2000) == 0)) || ((*param_1 & 1) != 1)) {
    iVar2 = FUN_0001cd28();
  }
  else {
    while (((iVar2 = FUN_000237a8(param_1[1],param_2,param_3,param_4), iVar2 == -1 &&
            (*DAT_00023074 == 2)) && ((*param_1 & 0x4000) == 0))) {
      FUN_00020af8(uVar1);
      FUN_0001cad0();
      FUN_00020ae4();
    }
  }
  FUN_00020af8(uVar1);
  return iVar2;
}

