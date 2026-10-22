// Function: FUN_0602b760
// Address:  0602b760
// Signature: undefined FUN_0602b760()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0602b760(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  *DAT_0602be74 = *DAT_0602be74 & 0x7f;
  _DAT_040001a0 = 0x8000;
  FUN_0602bc00();
  FUN_0602bb94();
  _DAT_040001a4 = DAT_0602be78;
  FUN_0602bd94(0x90000000,0,4,DAT_0602be7c,param_1,param_2,param_3,param_4);
  FUN_0602bd94(0,0,0x1000,DAT_0602be80);
  FUN_0602c1a8(DAT_0602be84,DAT_0602be88,0x160);
  if ((*(byte *)(DAT_0602be8c + 0x12) & 2) == 0) {
    FUN_0602c104(DAT_0602be90,0xea0);
    *DAT_0602be94 = 3;
  }
  return CONCAT44(param_2,param_1);
}

