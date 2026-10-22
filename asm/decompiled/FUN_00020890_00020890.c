// Function: FUN_00020890
// Address: 00020890


uint FUN_00020890(uint param_1)

{
  uint uVar1;
  uint uVar2;
  int unaff_r10;
  
  uVar1 = (uint)*(byte *)(unaff_r10 + (param_1 >> 0x18)) ^ param_1 << 8;
  uVar2 = (uint)*(byte *)(unaff_r10 + ((param_1 & 0xffffff) >> 0x10)) ^ uVar1 << 8;
  return (uint)*(byte *)(unaff_r10 + ((uVar2 & 0xffffff) >> 0x10)) ^
         ((uint)*(byte *)(unaff_r10 + ((uVar1 & 0xffffff) >> 0x10)) ^ uVar2 << 8) << 8;
}

