// Function: FUN_06027a70
// Address:  06027a70
// Signature: undefined FUN_06027a70()


void FUN_06027a70(void)

{
  int iVar1;
  
  iVar1 = 0x1000;
  do {
    iVar1 = iVar1 + -1;
    if (iVar1 == 0) {
      return;
    }
    FUN_06027a58();
  } while ((*DAT_060282dc & 1) == 0);
  FUN_06027ff0(DAT_060282e0,DAT_060282e4,1);
  return;
}

