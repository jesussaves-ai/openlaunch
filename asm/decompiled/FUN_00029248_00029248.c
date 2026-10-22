// Function: FUN_00029248
// Address: 00029248


int FUN_00029248(undefined4 param_1,int param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  int iVar2;
  
  iVar2 = param_3;
  while( true ) {
    if (iVar2 == 0) {
      return param_3;
    }
    iVar1 = FUN_00022be0(param_1,param_2,iVar2,param_4);
    if ((iVar1 == 0) || (iVar1 == -1)) break;
    iVar2 = iVar2 - iVar1;
    param_2 = param_2 + iVar1;
  }
  return 0;
}

