// Function: FUN_00024c60
// Address: 00024c60


void FUN_00024c60(void)

{
  if ((*(byte *)(DAT_0002521c + 0x84) & 0x80) == 0) {
    *DAT_00025218 = 0xffffffff;
  }
  else {
    *DAT_00025218 = 4;
    *DAT_00025220 = 2;
  }
  return;
}

