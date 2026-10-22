// Function: FUN_000236e4
// Address: 000236e4


undefined4 FUN_000236e4(int param_1,undefined4 param_2,undefined2 param_3)

{
  undefined2 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  
  if (param_1 == 0) {
    *DAT_0001ced0 = 1;
    return 0xffffffff;
  }
  uVar2 = FUN_00020ae4();
  if (*(int *)(param_1 + 4) == 0) {
    uVar3 = FUN_000222fc(0,param_2);
    *(undefined4 *)(param_1 + 0x28) = uVar3;
    FUN_00023108();
    uVar1 = FUN_00022078();
    *(undefined2 *)(param_1 + 0x30) = uVar1;
  }
  else {
    if (*(int *)(param_1 + 4) != 1) {
      uVar3 = FUN_0001cd28();
      goto LAB_00023794;
    }
    uVar3 = FUN_000222fc(*(undefined4 *)(param_1 + 0x28),param_2);
    *(undefined4 *)(param_1 + 0x28) = uVar3;
  }
  *(undefined4 *)(param_1 + 0x2c) = param_2;
  *(undefined2 *)(param_1 + 0x32) = param_3;
  uVar3 = FUN_000230b8(*(undefined4 *)(param_1 + 0x28),*(undefined4 *)(param_1 + 0x2c),
                       *(undefined2 *)(param_1 + 0x30),*(undefined2 *)(param_1 + 0x32));
  *(undefined4 *)(param_1 + 8) = uVar3;
  FUN_0002326c(param_1,2,0);
  *(undefined4 *)(param_1 + 0x24) = 0;
  *(undefined4 *)(param_1 + 4) = 3;
  uVar3 = 0;
LAB_00023794:
  FUN_00020af8(uVar2);
  return uVar3;
}

