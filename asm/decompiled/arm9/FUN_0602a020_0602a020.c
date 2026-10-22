// Function: FUN_0602a020
// Address:  0602a020
// Signature: undefined FUN_0602a020()


void FUN_0602a020(void)

{
  do {
    if ((*(uint *)(DAT_0602a398 + 0x1c) & 0x7f0000) != 0) {
      return;
    }
  } while ((*(uint *)(DAT_0602a398 + 0x1c) & 4) == 0);
  return;
}

