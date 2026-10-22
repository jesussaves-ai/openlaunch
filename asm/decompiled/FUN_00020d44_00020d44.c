// Function: FUN_00020d44
// Address: 00020d44


void FUN_00020d44(int param_1)

{
  uint uVar1;
  bool bVar2;
  
  *DAT_000214ec = *DAT_000214ec + param_1;
  uVar1 = *DAT_000214f0 + param_1;
  if (199 < uVar1) {
    uVar1 = 100;
  }
  bVar2 = 99 < uVar1;
  if (bVar2) {
    uVar1 = uVar1 - 100;
  }
  *DAT_000214f0 = uVar1;
  if (bVar2) {
    FUN_00020f0c();
  }
  uVar1 = *DAT_000214f4 + param_1;
  if (1999 < uVar1) {
    uVar1 = 1000;
  }
  bVar2 = 999 < uVar1;
  if (bVar2) {
    uVar1 = uVar1 - 1000;
  }
  *DAT_000214f4 = uVar1;
  if (bVar2) {
    FUN_000226fc();
  }
  FUN_00023acc();
  return;
}

