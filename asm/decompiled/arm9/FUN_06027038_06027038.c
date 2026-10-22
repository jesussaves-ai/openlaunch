// Function: FUN_06027038
// Address:  06027038
// Signature: undefined FUN_06027038()


undefined1 FUN_06027038(undefined4 param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  int iVar2;
  
  puVar1 = DAT_060274e4;
  do {
  } while ((DAT_060274e4[1] & 0x80) != 0);
  iVar2 = 0x180;
  do {
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  DAT_060274e4[1] = param_2;
  do {
  } while ((puVar1[1] & 0x80) != 0);
  return *puVar1;
}

