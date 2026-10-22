// Function: FUN_0002a16c
// Address: 0002a16c


undefined1 FUN_0002a16c(undefined4 param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  int iVar2;
  
  puVar1 = DAT_0002a5d0;
  do {
  } while ((DAT_0002a5d0[1] & 0x80) != 0);
  iVar2 = 0x180;
  do {
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  DAT_0002a5d0[1] = param_2;
  do {
  } while ((puVar1[1] & 0x80) != 0);
  return *puVar1;
}

