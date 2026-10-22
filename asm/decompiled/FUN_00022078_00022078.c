// Function: FUN_00022078
// Address: 00022078


uint FUN_00022078(uint param_1)

{
  return (param_1 | param_1 << 0x10) >> 8 & 0xff00ffff;
}

