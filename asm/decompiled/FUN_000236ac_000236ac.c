// Function: FUN_000236ac
// Address: 000236ac


undefined4 FUN_000236ac(int param_1)

{
  undefined4 uVar1;
  
  if (param_1 == 0) {
    *DAT_0001ced0 = 1;
    return 0xffffffff;
  }
  uVar1 = FUN_00020ae4();
  if (*(int *)(param_1 + 0x40) == 0) {
    *(undefined4 *)(param_1 + 0x40) = 1;
  }
  FUN_00020af8(uVar1);
  return 0;
}

