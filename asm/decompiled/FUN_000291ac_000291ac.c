// Function: FUN_000291ac
// Address: 000291ac


void FUN_000291ac(void)

{
  int *piVar1;
  undefined4 uVar2;
  int iVar3;
  
  uVar2 = FUN_00020ae4();
  piVar1 = DAT_00029bb4;
  if (*DAT_00029bb0 != 0) {
    iVar3 = *DAT_00029bb4;
    *DAT_00029bb4 = *DAT_00029bb0;
    if (*DAT_00029bb8 != 0) {
      FUN_00029914(*(undefined1 *)(*DAT_00029bb8 + 0x40));
      FUN_00029840();
      FUN_00029840();
      *piVar1 = iVar3;
    }
  }
  FUN_00020af8(uVar2);
  return;
}

