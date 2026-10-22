// Function: FUN_06028f88
// Address:  06028f88
// Signature: undefined FUN_06028f88()


undefined8 FUN_06028f88(int param_1,undefined4 param_2)

{
  ushort uVar1;
  undefined2 uVar2;
  undefined4 uVar3;
  int extraout_r1;
  undefined1 in_ZR;
  
  FUN_06028cb4();
  if ((bool)in_ZR) {
    uVar3 = DAT_0602978c;
    if (param_1 != 4) {
      uVar3 = DAT_06029790;
    }
    FUN_060287b8(uVar3,0x506);
  }
  else {
    uVar3 = FUN_06028ca0();
    FUN_060287b8(uVar3,0x37);
    if (param_1 == 4) {
      uVar3 = 2;
    }
    else {
      uVar3 = 0;
    }
    FUN_060287b8(uVar3,0x46);
  }
  if (param_1 == 4) {
    uVar1 = *(ushort *)(DAT_06029794 + 0x28) & 0x7fff;
  }
  else {
    uVar1 = *(ushort *)(DAT_06029794 + 0x28) | 0x8000;
  }
  *(ushort *)(DAT_06029794 + 0x28) = uVar1;
  uVar2 = FUN_06028838();
  *(undefined2 *)(extraout_r1 + 0x3e) = uVar2;
  return CONCAT44(param_2,param_1);
}

