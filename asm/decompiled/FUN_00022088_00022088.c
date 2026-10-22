// Function: FUN_00022088
// Address: 00022088


uint FUN_00022088(uint param_1)

{
  return param_1 << 0x18 | param_1 >> 0x18 | (param_1 & 0xff0000) >> 8 | (param_1 & 0xff00) << 8;
}

