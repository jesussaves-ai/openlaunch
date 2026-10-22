// Function: FUN_00024888
// Address: 00024888


void FUN_00024888(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_0001cd20();
  *DAT_00024a78 = puVar1;
  if (puVar1 != (undefined4 *)0x0) {
    *DAT_00024a7c = puVar1;
    *(undefined2 *)(puVar1 + 1) = 1;
    *puVar1 = 0;
    puVar1[2] = param_1 + -0xc;
  }
  return;
}

