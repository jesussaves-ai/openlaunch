// Function: FUN_06026ff8
// Address:  06026ff8
// Signature: undefined FUN_06026ff8()


void FUN_06026ff8(undefined1 param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  int iVar2;
  
  puVar1 = DAT_060274e0;
  do {
  } while ((DAT_060274e0[1] & 0x80) != 0);
  iVar2 = 0x180;
  do {
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *DAT_060274e0 = param_1;
  puVar1[1] = param_2;
  do {
  } while ((puVar1[1] & 0x80) != 0);
  return;
}

