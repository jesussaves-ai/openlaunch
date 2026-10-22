// Function: FUN_00022f18
// Address: 00022f18


undefined4 FUN_00022f18(uint *param_1)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  uVar1 = FUN_00020ae4();
  if (((param_1 == (uint *)0x0) || ((*param_1 & 0x2000) == 0)) || ((*param_1 & 1) != 1)) {
    uVar2 = FUN_0001cd28();
  }
  else {
    uVar2 = FUN_000236ac(param_1[1]);
  }
  FUN_00020af8(uVar1);
  return uVar2;
}

