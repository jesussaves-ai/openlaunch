// Function: FUN_00021d78
// Address: 00021d78


ushort * FUN_00021d78(ushort param_1,undefined4 param_2,code *param_3)

{
  ushort *puVar1;
  int iVar2;
  
  iVar2 = 1;
  puVar1 = DAT_00022314;
  do {
    if ((*puVar1 & 1) == 0) {
      *puVar1 = (ushort)DAT_00022318;
      puVar1[1] = param_1;
      *(undefined4 *)(puVar1 + 2) = param_2;
      if (param_3 != (code *)0x0) {
        (*param_3)(puVar1);
      }
      *DAT_0002231c = *DAT_0002231c + 1;
      return puVar1;
    }
    puVar1 = puVar1 + 4;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return (ushort *)0x0;
}

