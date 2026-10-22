// Function: FUN_00023164
// Address: 00023164


void FUN_00023164(int param_1)

{
  ushort uVar1;
  int iVar2;
  
  if (param_1 != 0) {
    iVar2 = *(int *)(param_1 + 0xc);
    *(undefined2 *)(iVar2 + 0x10) = 0;
    uVar1 = FUN_0002249c();
    uVar1 = ~uVar1;
    if (uVar1 == 0) {
      uVar1 = 0xffff;
    }
    *(ushort *)(iVar2 + 0x10) = uVar1;
  }
  return;
}

