// Function: FUN_000230b8
// Address: 000230b8


uint FUN_000230b8(int param_1,uint param_2,int param_3,int param_4)

{
  int iVar1;
  
  iVar1 = *DAT_00023a6c;
  return (DAT_00023a70 + iVar1 * 8) * param_1 ^ param_2 ^ (DAT_00023a74 + iVar1 * 0x20) * param_2 ^
         (DAT_00023a78 + iVar1 * 2) * param_3 ^ (DAT_00023a7c + iVar1) * param_4;
}

