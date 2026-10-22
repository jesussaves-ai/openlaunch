// Function: FUN_00020ec4
// Address: 00020ec4


bool FUN_00020ec4(int param_1,uint param_2)

{
  return (*(uint *)(param_1 + 0x18) | param_2) == 0xffffffff;
}

