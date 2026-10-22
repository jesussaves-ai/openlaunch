// Function: FUN_0002a068
// Address: 0002a068


void FUN_0002a068(undefined4 param_1,char *param_2)

{
  char *pcVar1;
  char *extraout_r1;
  undefined1 uVar2;
  
  while( true ) {
    pcVar1 = param_2 + 1;
    uVar2 = 0;
    if (*param_2 == '\0') break;
    do {
      FUN_0001c978(*pcVar1,pcVar1 + 1);
      pcVar1 = extraout_r1;
      param_2 = extraout_r1;
    } while (!(bool)uVar2);
  }
  return;
}

