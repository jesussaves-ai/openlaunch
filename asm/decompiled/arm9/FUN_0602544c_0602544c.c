// Function: FUN_0602544c
// Address:  0602544c
// Signature: undefined FUN_0602544c()


undefined8 FUN_0602544c(undefined4 param_1,undefined4 param_2)

{
  do {
    software_interrupt(0x60000);
  } while (*DAT_060256c8 == *DAT_060256c8);
  return CONCAT44(param_2,param_1);
}

