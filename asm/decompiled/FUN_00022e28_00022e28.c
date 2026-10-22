// Function: FUN_00022e28
// Address: 00022e28


int FUN_00022e28(uint *param_1,undefined2 *param_2,undefined4 *param_3)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_00020ae4();
  if (((((param_1 == (uint *)0x0 || param_2 == (undefined2 *)0x0) || param_3 == (undefined4 *)0x0)
       || ((*param_1 & 0x2000) == 0)) || ((*param_1 & 1) != 1)) ||
     (iVar2 = FUN_00022804(*param_1 & 0x4000 | 1), iVar2 == -1)) {
    iVar2 = FUN_0001cd28();
  }
  else {
    while (iVar3 = FUN_00023628(param_1[1]), iVar3 == 0) {
      if ((*DAT_00023080 != 2) || ((*param_1 & 0x4000) != 0)) {
        FUN_00022860(iVar2);
        iVar2 = -1;
        goto LAB_00022e6c;
      }
      FUN_00020af8(uVar1);
      FUN_0001cad0();
      FUN_00020ae4();
    }
    *param_3 = 0x10;
    *param_2 = 2;
    *(undefined4 *)(param_2 + 2) = *(undefined4 *)(iVar3 + 0x2c);
    param_2[1] = *(undefined2 *)(iVar3 + 0x32);
    *(int *)(iVar2 + 4) = iVar3;
  }
LAB_00022e6c:
  FUN_00020af8(uVar1);
  return iVar2;
}

