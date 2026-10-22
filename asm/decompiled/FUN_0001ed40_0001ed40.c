// Function: FUN_0001ed40
// Address: 0001ed40


void FUN_0001ed40(char *param_1,int param_2)

{
  undefined1 uVar1;
  
  if (*param_1 == '\x03') {
    uVar1 = 2;
  }
  else {
    uVar1 = 4;
  }
  *(undefined1 *)(param_2 + 3) = uVar1;
  return;
}

