// Function: FUN_0002995c
// Address: 0002995c


undefined8 FUN_0002995c(undefined4 param_1,undefined1 *param_2)

{
  undefined1 *extraout_r1;
  undefined1 *extraout_r1_00;
  undefined1 *puVar1;
  int iVar2;
  
  iVar2 = 6;
  puVar1 = param_2;
  do {
    FUN_000298c8(*puVar1,puVar1 + 1);
    puVar1 = extraout_r1;
    if (iVar2 != 1) {
      FUN_000296b4(0x3a);
      puVar1 = extraout_r1_00;
    }
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return CONCAT44(param_2,param_1);
}

