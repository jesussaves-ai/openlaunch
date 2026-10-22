// Function: FUN_0602a72c
// Address:  0602a72c
// Signature: undefined FUN_0602a72c()


void FUN_0602a72c(uint param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = DAT_0602ad78;
  *DAT_0602ad7c = DAT_0602ad78;
  *DAT_0602ad80 = uVar1;
  if ((param_1 & 0x40) != 0) {
    FUN_0602a830();
  }
  if ((param_1 & 8) != 0) {
    FUN_0602a818();
  }
  if ((param_1 & 4) != 0) {
    FUN_0602a810();
  }
  if ((param_1 & 0x20) != 0) {
    FUN_0602a820();
  }
  if ((param_1 & 0x10) != 0) {
    FUN_0602a828();
  }
  if ((param_1 & 0x80) != 0) {
    FUN_0602a838();
  }
  if (((param_1 & 1) != 0) && (iVar2 = FUN_0602a3c4(DAT_0602ad84), iVar2 != 0)) {
    FUN_0602a840();
  }
  if (((param_1 & 2) != 0) && (iVar2 = FUN_0602a3c4(DAT_0602ad88), iVar2 != 0)) {
    FUN_0602a840();
  }
  *DAT_0602ad90 = *DAT_0602ad8c;
  return;
}

