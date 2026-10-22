// Function: FUN_0002a12c
// Address: 0002a12c


void FUN_0002a12c(undefined1 param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  int iVar2;
  
  puVar1 = DAT_0002a5cc;
  do {
  } while ((DAT_0002a5cc[1] & 0x80) != 0);
  iVar2 = 0x180;
  do {
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *DAT_0002a5cc = param_1;
  puVar1[1] = param_2;
  do {
  } while ((puVar1[1] & 0x80) != 0);
  return;
}

