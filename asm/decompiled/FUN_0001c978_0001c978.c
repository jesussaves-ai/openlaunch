// Function: FUN_0001c978
// Address: 0001c978


undefined1 FUN_0001c978(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = DAT_0001ce84;
  DAT_04000208 = 0;
  uVar2 = DAT_0001ce80;
  if (param_3 != 1) {
    uVar2 = DAT_0001ce80 | 0x800;
  }
  *(uint *)(DAT_0001ce84 + 0x1c0) = uVar2 | param_1 << 0x10;
  do {
  } while ((*(byte *)(iVar1 + 0x1c0) & 0x80) != 0);
  if (param_3 == 1) {
    FUN_0001cefc(3);
  }
  if (param_3 == 1) {
    DAT_04000208 = 1;
  }
  return *(undefined1 *)(iVar1 + 0x1c2);
}

