// Function: FUN_0001cfdc
// Address: 0001cfdc


undefined8 FUN_0001cfdc(undefined4 param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined1 *extraout_r1;
  undefined1 *puVar2;
  undefined1 *extraout_r1_00;
  
  iVar1 = FUN_0001cf1c();
  FUN_0001cf54(iVar1 << 4 | 6);
  puVar2 = extraout_r1;
  do {
    FUN_0001cf54(*puVar2,puVar2 + 1);
    param_3 = param_3 + -1;
    puVar2 = extraout_r1_00;
  } while (param_3 != 0);
  FUN_0001cf40();
  return CONCAT44(param_2,param_1);
}

