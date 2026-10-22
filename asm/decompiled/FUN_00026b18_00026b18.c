// Function: FUN_00026b18
// Address: 00026b18


void FUN_00026b18(ushort *param_1)

{
  ushort uVar1;
  int iVar2;
  ushort *puVar3;
  
  iVar2 = DAT_00026e34;
  while( true ) {
    puVar3 = param_1 + 1;
    uVar1 = *param_1;
    param_1 = param_1 + 2;
    if (uVar1 == 0) break;
    *(ushort *)(iVar2 + (uint)uVar1) = *puVar3;
  }
  return;
}

