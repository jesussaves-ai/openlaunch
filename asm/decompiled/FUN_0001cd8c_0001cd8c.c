// Function: FUN_0001cd8c
// Address: 0001cd8c


undefined8 FUN_0001cd8c(uint param_1,undefined4 param_2)

{
  return CONCAT44(param_2,((param_1 ^ param_1 & 0xff00ff00) >> 8 | param_1 << 0x18) +
                          (param_1 >> 0x18 | (param_1 & 0xff00ff00) << 8));
}

