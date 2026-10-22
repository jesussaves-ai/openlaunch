// Function: FUN_0002596c
// Address: 0002596c


undefined4 FUN_0002596c(uint param_1)

{
  undefined4 uVar1;
  uint uVar2;
  
  FUN_0001c72c(DAT_00025ccc,DAT_00025cd0);
  FUN_0001c72c(DAT_00025cd4,DAT_00025cd8);
  FUN_0001cae0();
  uVar1 = DAT_00025cdc;
  *DAT_00025ce0 = DAT_00025cdc;
  uVar2 = param_1 & 0xf000;
  if (uVar2 == 0x1000) {
    uVar1 = 0x10000;
  }
  if (uVar2 == 0) {
    uVar1 = 0x20000;
  }
  if (uVar2 == 0x2000) {
    uVar1 = 0x40000;
  }
  if (uVar2 == 0x3000) {
    uVar1 = 0x80000;
  }
  if (uVar2 != 0x4000) {
    FUN_00024888(uVar1);
  }
  FUN_00020d04();
  *DAT_00025ce4 = param_1 & 2 | 1;
  return DAT_00025ce8;
}

