// Function: FUN_00027a8c
// Address: 00027a8c


void FUN_00027a8c(ushort param_1,undefined4 *param_2)

{
  undefined2 *extraout_r1;
  
  *param_2 = 0;
  param_2[1] = 0;
  param_2[2] = 10;
  *(ushort *)(param_2 + 3) = param_1;
  *(undefined2 *)((int)param_2 + 0xe) = 0;
  FUN_00025f60(DAT_0002825c,param_2 + 4);
  FUN_00025f60(DAT_00028260);
  FUN_00025f60(DAT_00028264);
  *extraout_r1 = 0;
  if ((param_1 & 0x4000) != 0) {
    *(uint *)(uint)*DAT_00028270 =
         ((uint)*(ushort *)(DAT_00028268 + 0x44) ^ (uint)*(ushort *)(DAT_00028268 + 0x44) << 7) &
         DAT_0002826c | (int)(uint)*DAT_00028270 << 0x1e;
  }
  return;
}

