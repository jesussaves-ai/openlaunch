// Function: FUN_06026304
// Address:  06026304
// Signature: undefined FUN_06026304()


void FUN_06026304(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  
  puVar1 = DAT_06026768;
  *DAT_0602676c = DAT_06026768;
  do {
    puVar2 = puVar1 + 1;
    *puVar1 = 0x20;
    puVar1 = puVar2;
  } while (((uint)puVar2 & 0x800) == 0);
  return;
}

