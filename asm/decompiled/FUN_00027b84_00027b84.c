// Function: FUN_00027b84
// Address: 00027b84


void FUN_00027b84(undefined4 param_1,undefined4 param_2)

{
  undefined2 *extraout_r1;
  int extraout_r1_00;
  undefined1 auStack_14c [320];
  
  FUN_00027a8c(DAT_00028274,auStack_14c);
  *extraout_r1 = 1;
  extraout_r1[1] = 3;
  extraout_r1[2] = 0;
  *(undefined1 *)(extraout_r1 + 3) = 0x10;
  *(char *)((int)extraout_r1 + 7) = (char)param_1;
  FUN_0001c7d0(param_2,extraout_r1 + 4,param_1);
  FUN_000279fc(auStack_14c,extraout_r1_00 - (int)auStack_14c);
  return;
}

