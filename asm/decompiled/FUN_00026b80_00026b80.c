// Function: FUN_00026b80
// Address: 00026b80


void FUN_00026b80(void)

{
  uint uVar1;
  
  if (*DAT_00026e40 == '\x02') {
    FUN_00026928(DAT_00026e44);
  }
  uVar1 = FUN_000268cc(0x1e);
  FUN_000268f8(0x1e,uVar1 | 0x3f);
  *(short *)(DAT_00026e48 + 0x68) = (short)DAT_00026e4c;
  *(undefined2 *)(DAT_00026e50 + 0x36) = 1;
  return;
}

