// Function: FUN_0001c92c
// Address: 0001c92c


undefined8 FUN_0001c92c(uint param_1,undefined1 *param_2)

{
  uint uVar1;
  uint uVar2;
  
  *param_2 = (char)param_1;
  param_2[1] = (char)(param_1 >> 8);
  uVar1 = (param_1 >> 8 | param_1 << 0x18) >> 8;
  param_2[2] = (char)(param_1 >> 0x10);
  uVar2 = (uVar1 | (param_1 >> 8) << 0x18) >> 8;
  param_2[3] = (char)(param_1 >> 0x18);
  return CONCAT44(param_2 + 4,(uVar2 | uVar1 << 0x18) >> 8 | uVar2 << 0x18);
}

