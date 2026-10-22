// Function: FUN_000235b8
// Address: 000235b8


undefined4 FUN_000235b8(int param_1,int param_2)

{
  undefined4 uVar1;
  undefined4 *puVar2;
  
  if ((param_1 != 0) && (*(int *)(param_1 + 4) == 1)) {
    if (param_2 == 0) {
      param_2 = 1;
    }
    uVar1 = FUN_00020ae4();
    if ((*(int *)(param_1 + 0x38) == 0) &&
       (puVar2 = (undefined4 *)FUN_000248c8(param_2 << 2), puVar2 != (undefined4 *)0x0)) {
      *(undefined4 **)(param_1 + 0x34) = puVar2;
      *puVar2 = 0;
      *(int *)(param_1 + 0x38) = param_2;
      *(undefined4 *)(param_1 + 4) = 2;
    }
    FUN_00020af8(uVar1);
    return 0;
  }
  *DAT_0001ced0 = 1;
  return 0xffffffff;
}

