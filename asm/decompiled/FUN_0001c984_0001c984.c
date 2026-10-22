// Function: FUN_0001c984
// Address: 0001c984


undefined1 FUN_0001c984(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined *puVar2;
  
  iVar1 = DAT_0001ce84;
  puVar2 = (undefined *)0x8100;
  DAT_04000208 = 0;
  if (param_3 != 1) {
    puVar2 = &DAT_00008900;
  }
  *(uint *)(DAT_0001ce84 + 0x1c0) = (uint)puVar2 | param_1 << 0x10;
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

