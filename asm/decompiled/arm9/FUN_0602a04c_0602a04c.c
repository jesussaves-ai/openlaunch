// Function: FUN_0602a04c
// Address:  0602a04c
// Signature: undefined FUN_0602a04c()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0602a04c(undefined4 param_1,undefined2 *param_2,uint param_3)

{
  int iVar1;
  
  iVar1 = DAT_0602a39c;
  if (((*(ushort *)(DAT_0602a39c + 0xd8) & 2) == 0) || ((*(uint *)(DAT_0602a39c + 0x100) & 2) == 0))
  {
    do {
      if ((*(uint *)(DAT_0602a39c + 0x1c) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_0602a39c + 0x1c) & 0x1000000) == 0);
    do {
      *param_2 = *(undefined2 *)(iVar1 + 0x30);
      param_3 = param_3 - 2;
      param_2 = param_2 + 1;
    } while (param_3 != 0);
  }
  else {
    do {
      if ((*(uint *)(DAT_0602a39c + 0x1c) & 0x7f0000) != 0) {
        return 0xffffffff;
      }
    } while ((*(uint *)(DAT_0602a39c + 0x100) & 0x100) == 0);
    _DAT_040000d4 = DAT_0602a39c + 0x10c;
    _DAT_040000dc = param_3 >> 2 | 0x85000000;
    _DAT_040000d8 = param_2;
  }
  return 0;
}

