// Function: FUN_000264c0
// Address: 000264c0


undefined2 FUN_000264c0(int param_1)

{
  uint uVar1;
  
  uVar1 = *DAT_00026da8 + param_1;
  if (0x2fff < uVar1) {
    uVar1 = uVar1 - 0x3000;
  }
  return *(undefined2 *)(DAT_00026dac + uVar1 * 2);
}

