// Function: FUN_0602bc3c
// Address:  0602bc3c
// Signature: undefined FUN_0602bc3c()


undefined4 FUN_0602bc3c(int param_1)

{
  do {
    software_interrupt(0x30000);
    param_1 = param_1 + -1;
  } while (param_1 != 0);
  return DAT_0602bf28;
}

