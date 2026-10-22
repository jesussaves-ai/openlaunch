// Function: FUN_060255f0
// Address:  060255f0
// Signature: undefined FUN_060255f0()


void FUN_060255f0(void)

{
  FUN_06025624();
  do {
    software_interrupt(0x30000);
  } while (1 < *DAT_06025700);
  if (*DAT_06025700 == 1) {
    FUN_06026f8c(0,0x12);
  }
  return;
}

