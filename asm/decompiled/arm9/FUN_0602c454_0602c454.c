// Function: FUN_0602c454
// Address:  0602c454
// Signature: undefined FUN_0602c454()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0602c454(void)

{
  undefined4 uVar1;
  
  _DAT_04000304 = DAT_0602c4fc;
  if (*DAT_0602c500 != 0) {
    do {
    } while ((*DAT_0602c504 & 0x40) == 0);
  }
  FUN_06026168();
  FUN_06026254();
  FUN_060261b0();
  FUN_060262c0();
  uVar1 = FUN_06025df4();
  uVar1 = FUN_060264a0(uVar1,DAT_0602c508);
  *DAT_0602c510 = DAT_0602c50c;
  uVar1 = FUN_060264a0(uVar1,DAT_0602c514);
  *DAT_0602c51c = DAT_0602c518;
  FUN_060264a0(uVar1,DAT_0602c520);
  return;
}

