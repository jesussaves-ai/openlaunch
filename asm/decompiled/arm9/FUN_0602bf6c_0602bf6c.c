// Function: FUN_0602bf6c
// Address:  0602bf6c
// Signature: undefined FUN_0602bf6c()


undefined8 FUN_0602bf6c(uint param_1,undefined4 param_2)

{
  return CONCAT44(param_2,((param_1 ^ param_1 & 0xff00ff00) >> 8 | param_1 << 0x18) +
                          (param_1 >> 0x18 | (param_1 & 0xff00ff00) << 8));
}

