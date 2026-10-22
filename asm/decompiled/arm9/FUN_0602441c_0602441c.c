// Function: FUN_0602441c
// Address:  0602441c
// Signature: undefined FUN_0602441c()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_0602441c(void)

{
  uint uVar1;
  uint uVar2;
  
  uVar2 = (uint)*(ushort *)(DAT_060250cc + 0x10) << 0x1f |
          (*(uint *)(DAT_060250cc + 0x81c) & 0x20) << 0x19;
  uVar1 = *DAT_060250d0;
  *DAT_060250d0 = uVar2;
  uVar2 = uVar2 ^ uVar1;
  if ((_DAT_04000134 & 0x800000) != 0) {
    uVar2 = uVar2 | 0x20000000;
  }
  return ~(_DAT_04000134 & 0x30000 | _DAT_04000130 & 0x3ff) & DAT_060250d4 | uVar2;
}

