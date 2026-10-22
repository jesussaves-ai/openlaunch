// Function: FUN_00024ea0
// Address: 00024ea0


void FUN_00024ea0(int param_1,int param_2)

{
  undefined1 uVar1;
  undefined1 unaff_r8;
  undefined1 unaff_r9;
  undefined1 in_ZR;
  
  uVar1 = 2;
  FUN_00024f1c();
  if ((bool)in_ZR) {
    uVar1 = 1;
    FUN_00024f1c();
    if ((bool)in_ZR) {
      if (*(int *)(param_1 + 0x58) == 0 && *(int *)(param_1 + 0x5c) == 0) {
        uVar1 = 0;
        if ((*(ushort *)(param_1 + 0x36) & 2) == 0) {
          unaff_r8 = 1;
          unaff_r9 = unaff_r8;
        }
        else {
          unaff_r8 = 2;
          unaff_r9 = unaff_r8;
        }
      }
      else {
        uVar1 = 3;
        unaff_r8 = 0;
        unaff_r9 = 0;
      }
    }
  }
  *(undefined1 *)(param_2 + 0x87) = uVar1;
  *(undefined1 *)(param_2 + 0x88) = unaff_r8;
  *(undefined1 *)(param_2 + 0x89) = unaff_r9;
  return;
}

