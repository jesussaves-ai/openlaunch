// Function: FUN_06026e08
// Address:  06026e08
// Signature: undefined FUN_06026e08()


uint FUN_06026e08(void)

{
  uint uVar1;
  uint *puVar2;
  int iVar3;
  int unaff_r11;
  
  iVar3 = 0x800;
  uVar1 = 0;
  puVar2 = *(uint **)(unaff_r11 + 0x28);
  do {
    uVar1 = uVar1 | *puVar2;
    iVar3 = iVar3 + -4;
    puVar2 = puVar2 + 1;
  } while (iVar3 != 0);
  return uVar1;
}

