// Function: FUN_00025f60
// Address: 00025f60


undefined8 FUN_00025f60(undefined2 *param_1,undefined2 *param_2)

{
  *param_2 = *param_1;
  param_2[1] = param_1[1];
  param_2[2] = param_1[2];
  return CONCAT44(param_2 + 3,param_1 + 3);
}

