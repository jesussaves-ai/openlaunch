// Function: FUN_0602a03c
// Address:  0602a03c
// Signature: undefined FUN_0602a03c()


void FUN_0602a03c(void)

{
  FUN_0602a04c();
  do {
    if ((*(uint *)(DAT_0602a398 + 0x1c) & 0x7f0000) != 0) {
      return;
    }
  } while ((*(uint *)(DAT_0602a398 + 0x1c) & 4) == 0);
  return;
}

