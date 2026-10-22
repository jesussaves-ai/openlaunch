// Function: FUN_060262c0
// Address:  060262c0
// Signature: undefined FUN_060262c0()


void FUN_060262c0(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  
  puVar2 = DAT_06026758;
  do {
    *DAT_0602675c = puVar2;
    iVar3 = 0x400;
    puVar1 = puVar2;
    do {
      *puVar1 = 0x20;
      iVar3 = iVar3 + -1;
      puVar1 = puVar1 + 1;
    } while (iVar3 != 0);
    puVar2 = (undefined2 *)((uint)puVar2 ^ 0x200000);
  } while (((uint)puVar2 & 0x200000) != 0);
  *DAT_06026764 = DAT_06026760;
  return;
}

