// Function: FUN_0001c6bc
// Address: 0001c6bc


undefined4 FUN_0001c6bc(int param_1)

{
  uint uVar1;
  uint *puVar2;
  
  puVar2 = (uint *)(param_1 + 0x38);
  if (*puVar2 != 0) {
    return 1;
  }
  uVar1 = *puVar2;
  *puVar2 = 1;
  if (1 < uVar1) {
    *puVar2 = uVar1;
    return 1;
  }
  return 0;
}

