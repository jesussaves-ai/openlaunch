// Function: FUN_0602b2a4
// Address:  0602b2a4
// Signature: undefined FUN_0602b2a4()


undefined1 FUN_0602b2a4(undefined4 param_1,undefined1 param_2)

{
  undefined1 *puVar1;
  
  puVar1 = DAT_0602b5b8;
  do {
  } while ((DAT_0602b5b8[1] & 0x80) != 0);
  DAT_0602b5b8[1] = param_2;
  do {
  } while ((puVar1[1] & 0x80) != 0);
  return *puVar1;
}

