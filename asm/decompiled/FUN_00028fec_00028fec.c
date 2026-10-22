// Function: FUN_00028fec
// Address: 00028fec


undefined8 FUN_00028fec(undefined4 *param_1,int param_2)

{
  undefined4 *puVar1;
  int iVar2;
  
  puVar1 = param_1;
  iVar2 = param_2;
  do {
    *puVar1 = 0;
    puVar1[1] = 0;
    puVar1[2] = 0;
    puVar1[3] = 0;
    puVar1[4] = 0;
    puVar1[5] = 0;
    puVar1[6] = 0;
    puVar1[7] = 0;
    puVar1 = puVar1 + 8;
    iVar2 = iVar2 + -0x20;
  } while (iVar2 != 0);
  return CONCAT44(param_2,param_1);
}

