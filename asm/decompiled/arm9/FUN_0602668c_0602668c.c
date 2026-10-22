// Function: FUN_0602668c
// Address:  0602668c
// Signature: undefined FUN_0602668c()


int FUN_0602668c(int param_1)

{
  software_interrupt(0x30000);
  return param_1 << 3;
}

