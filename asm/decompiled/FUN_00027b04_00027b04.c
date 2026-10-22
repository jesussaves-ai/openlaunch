// Function: FUN_00027b04
// Address: 00027b04


void FUN_00027b04(void)

{
  undefined2 *extraout_r1;
  undefined1 auStack_44 [68];
  
  FUN_00027a8c(0xb0,auStack_44);
  *extraout_r1 = 0;
  extraout_r1[1] = 1;
  extraout_r1[2] = 0;
  FUN_000279fc(auStack_44,(int)extraout_r1 + (6 - (int)auStack_44));
  return;
}

