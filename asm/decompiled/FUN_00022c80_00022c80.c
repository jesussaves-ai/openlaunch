// Function: FUN_00022c80
// Address: 00022c80


undefined4 FUN_00022c80(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined4 uVar2;
  int unaff_r4;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 == (uint *)0x0 || unaff_r4 == 0) || ((*param_1 & 0x2000) == 0)) ||
     ((*param_1 & 1) != 0)) {
    uVar2 = FUN_0001cd28();
  }
  else {
    uVar2 = FUN_00024518(param_1[1],param_2,param_3,param_4,uVar1,param_1);
  }
  FUN_00020af8(uVar1);
  return uVar2;
}

