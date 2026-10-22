// Function: FUN_0602a840
// Address:  0602a840
// Signature: undefined FUN_0602a840()


void FUN_0602a840(void)

{
  ushort *puVar1;
  ushort *puVar2;
  
  *DAT_0602adb4 = DAT_0602adb0;
  puVar1 = (ushort *)*DAT_0602adb8;
  FUN_060291d8(*DAT_0602adbc);
  puVar2 = (ushort *)*DAT_0602adc0;
  for (; puVar1 < puVar2; puVar1 = (ushort *)((int)puVar1 + (uint)*puVar1)) {
    if ((*(byte *)((int)puVar1 + 3) & 0x10) != 0) {
      FUN_0602a4cc(puVar1 + 6,DAT_0602adc4);
      FUN_0602a4cc(puVar1 + 6,DAT_0602adc8);
      *DAT_0602adcc = *(undefined4 *)(puVar1 + 4);
      FUN_0602a840();
    }
  }
  *DAT_0602add0 = 0;
  return;
}

