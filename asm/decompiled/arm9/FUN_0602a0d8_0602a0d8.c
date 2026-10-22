// Function: FUN_0602a0d8
// Address:  0602a0d8
// Signature: undefined FUN_0602a0d8()


undefined4 FUN_0602a0d8(undefined4 param_1,undefined4 *param_2,int param_3)

{
  int iVar1;
  
  iVar1 = DAT_0602a3a0;
  do {
    if ((*(uint *)(DAT_0602a3a0 + 0x1c) & 0x7f0000) != 0) {
      return 0xffffffff;
    }
  } while ((*(uint *)(DAT_0602a3a0 + 0x100) & 0x200) != 0);
  do {
    *(undefined4 *)(iVar1 + 0x10c) = *param_2;
    param_3 = param_3 + -4;
    param_2 = param_2 + 1;
  } while (param_3 != 0);
  return 0;
}

