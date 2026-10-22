// Function: FUN_06029c90
// Address:  06029c90
// Signature: undefined FUN_06029c90()


undefined8 FUN_06029c90(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  undefined1 in_ZR;
  
  FUN_0602a57c(DAT_0602a300);
  FUN_0602946c();
  iVar2 = DAT_0602a304;
  do {
    iVar1 = *DAT_0602a30c;
    FUN_06029f04(*DAT_0602a308,iVar2);
    *DAT_0602a310 = 0;
    FUN_06029590();
    iVar2 = iVar2 + iVar1;
  } while (!(bool)in_ZR);
  return CONCAT44(param_2,param_1);
}

