// Function: FUN_00021dd8
// Address: 00021dd8


ushort * FUN_00021dd8(undefined4 param_1,code *param_2)

{
  ushort *puVar1;
  int iVar2;
  
  iVar2 = 1;
  puVar1 = DAT_00022320;
  do {
    if ((*puVar1 & 1) == 0) {
      *puVar1 = (ushort)DAT_00022324;
      *(undefined4 *)(puVar1 + 4) = param_1;
      if (param_2 != (code *)0x0) {
        (*param_2)(puVar1);
      }
      *DAT_00022328 = *DAT_00022328 + 1;
      return puVar1;
    }
    puVar1 = puVar1 + 0x1a;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return (ushort *)0x0;
}

