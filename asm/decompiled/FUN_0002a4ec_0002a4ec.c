// Function: FUN_0002a4ec
// Address: 0002a4ec


undefined8 FUN_0002a4ec(undefined4 param_1,uint *param_2)

{
  uint *unaff_r4;
  
  do {
  } while ((*unaff_r4 & 0x3e0) < 0x80);
  *param_2 = unaff_r4[3];
  param_2[1] = unaff_r4[3];
  param_2[2] = unaff_r4[3];
  param_2[3] = unaff_r4[3];
  return CONCAT44(param_2,param_1);
}

