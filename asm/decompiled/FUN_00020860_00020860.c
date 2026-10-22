// Function: FUN_00020860
// Address: 00020860


uint FUN_00020860(uint param_1)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int unaff_r11;
  
  uVar1 = *(uint *)(unaff_r11 + ((param_1 & 0xff00) >> 6));
  uVar2 = *(uint *)(unaff_r11 + ((param_1 & 0xff0000) >> 0xe));
  uVar3 = *(uint *)(unaff_r11 + ((param_1 & 0xff000000) >> 0x16));
  return *(uint *)(unaff_r11 + (param_1 & 0xff) * 4) ^ (uVar1 >> 0x18 | uVar1 << 8) ^
         (uVar2 >> 0x10 | uVar2 << 0x10) ^ (uVar3 >> 8 | uVar3 << 0x18);
}

