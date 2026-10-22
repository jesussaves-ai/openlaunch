// Function: FUN_06026950
// Address:  06026950
// Signature: undefined FUN_06026950()


undefined8 FUN_06026950(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = DAT_06027404;
  if (((*(byte *)(DAT_06027404 + 0x12) & 2) != 0) && (*(int *)(DAT_06027404 + 0x1d4) != 0)) {
    FUN_0602c104(*(int *)(DAT_06027404 + 0x1d4),0x400);
    if ((*(byte *)(iVar1 + 0x234) & 4) != 0) {
      FUN_06026a38(DAT_06027408,*(int *)(iVar1 + 0x1d4) + 0x3c0,0x40);
    }
    FUN_06026a18(DAT_0602740c,*(undefined4 *)(iVar1 + 0x1d4));
    if ((*(byte *)(iVar1 + 0x1b4) & 0x10) != 0) {
      if ((*(byte *)(iVar1 + 0x234) & 0x10) == 0) {
        DAT_06026a9d = 0;
      }
      else {
        DAT_06026a9d = 6;
      }
      DAT_06026aa8 = DAT_06026a9d;
      FUN_06026a18(DAT_06027410);
      FUN_06026a18(DAT_06027414);
      FUN_06026a18(DAT_06027418);
      FUN_06026a18(DAT_0602741c);
      if ((*(byte *)(iVar1 + 0x234) & 4) != 0) {
        if (*(int *)(iVar1 + 0x23c) != 0) {
          FUN_06026a18(DAT_06027420);
        }
        if (*(int *)(iVar1 + 0x238) != 0) {
          FUN_06026a18(DAT_06027424);
        }
      }
    }
  }
  return CONCAT44(param_2,param_1);
}

