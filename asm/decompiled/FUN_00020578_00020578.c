// Function: FUN_00020578
// Address: 00020578


void FUN_00020578(byte param_1,undefined4 param_2)

{
  byte *pbVar1;
  int iVar2;
  undefined4 uStack000000b4;
  
  iVar2 = 0x40;
  pbVar1 = (byte *)register0x00000054;
  do {
    *pbVar1 = *pbVar1 ^ param_1;
    iVar2 = iVar2 + -1;
    pbVar1 = pbVar1 + 1;
  } while (iVar2 != 0);
  uStack000000b4 = 0;
  FUN_0001fc7c(&stack0x00000054);
  FUN_0001fcd4(&stack0x00000054,&stack0x00000000,0x40);
  FUN_0001fcd4(&stack0x00000054);
  thunk_FUN_0001fe08(param_2,&stack0x00000054);
  return;
}

