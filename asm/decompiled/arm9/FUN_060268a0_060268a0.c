// Function: FUN_060268a0
// Address:  060268a0
// Signature: undefined FUN_060268a0()


undefined8 FUN_060268a0(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int unaff_r8;
  bool bVar2;
  
  iVar1 = 0x2000;
  do {
    bVar2 = (*(ushort *)(unaff_r8 + 0x80) & 1) == 0;
    if (!bVar2) {
      iVar1 = iVar1 + -1;
      bVar2 = iVar1 == 0;
    }
  } while (!bVar2);
  return CONCAT44(param_2,param_1);
}

