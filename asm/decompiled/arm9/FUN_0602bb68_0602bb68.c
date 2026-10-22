// Function: FUN_0602bb68
// Address:  0602bb68
// Signature: undefined FUN_0602bb68()


void FUN_0602bb68(void)

{
  uint uVar1;
  
  uVar1 = *(uint *)(DAT_0602bf18 + 0x20) & 0xfffff000;
  if ((0x3fff < uVar1) && (uVar1 < 0x8000)) {
    return;
  }
  return;
}

