// Function: FUN_0001fec4
// Address: 0001fec4


void FUN_0001fec4(int param_1)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  
  puVar1 = DAT_0002060c;
  iVar2 = FUN_00022088(*DAT_0002060c);
  uVar3 = FUN_00022088(puVar1[1]);
  uVar4 = FUN_00022088(iVar2 + (uint)(0xfffffffe < uVar3));
  *puVar1 = uVar4;
  uVar4 = FUN_00022088(uVar3 + 1);
  puVar1[1] = uVar4;
  FUN_0001caa8(param_1 + 9,puVar1,8);
  return;
}

