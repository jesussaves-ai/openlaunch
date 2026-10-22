// Function: FUN_0001cb38
// Address: 0001cb38


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0001cb38(uint param_1)

{
  int iVar1;
  
  iVar1 = DAT_0001ce90;
  *(uint *)(DAT_0001ce90 + 0x180) = *(uint *)(DAT_0001ce90 + 0x180) | 0x4000;
  while ((*(uint *)(iVar1 + 0x180) & 0xf) != param_1) {
    if ((_DAT_04000208 & 1) != 0) {
      FUN_0001cef4();
    }
  }
  *(uint *)(iVar1 + 0x180) = param_1 << 8 | 0x6000;
  return;
}

