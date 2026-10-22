// Function: FUN_00026954
// Address: 00026954


void FUN_00026954(uint *param_1)

{
  uint uVar1;
  
  if ((param_1 < 4) && (param_1 != (uint *)*DAT_00026e0c)) {
    *(uint *)*DAT_00026e0c = (uint)param_1;
    uVar1 = FUN_0001c9f4();
    FUN_0001c9d8(uVar1,uVar1 & 0xcf | (int)param_1 << 4);
  }
  return;
}

