// Function: FUN_0602b828
// Address:  0602b828
// Signature: undefined FUN_0602b828()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0602b828(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined8 uVar1;
  
  FUN_0602b920(0);
  FUN_0602bb68();
  FUN_0602b9d4();
  if ((*DAT_0602beac & 0x40000000) != 0) {
    FUN_0602bc58();
    _DAT_040001a4 = DAT_0602beb0;
    uVar1 = FUN_0602b920(1);
    FUN_0602b9d4((int)uVar1,(int)((ulonglong)uVar1 >> 0x20),0x4000,DAT_0602beb4);
  }
  FUN_0602bce4(0xa0000000,0,0,DAT_0602beb8,param_1,param_2,param_3,param_4);
  FUN_0602bc8c();
  FUN_0602bd94(0xb8000000,0,4,DAT_0602bebc);
  FUN_06027128();
  FUN_0602ba30(DAT_0602bec0);
  FUN_0602bb68(DAT_0602bec4,DAT_0602bec8);
  FUN_0602ba8c();
  FUN_060270f8();
  FUN_0602ba30(DAT_0602becc);
  if ((*(byte *)(DAT_0602bed0 + 0x12) & 2) != 0) {
    FUN_06027128();
    FUN_0602ba30(DAT_0602bed4);
    if ((*DAT_0602bed8 & 0x40000000) != 0) {
      FUN_0602ba8c(DAT_0602bedc,DAT_0602bee0,0x4000);
    }
    FUN_060270f8();
    FUN_0602ba30(DAT_0602bee4);
  }
  _DAT_040001a0 = 0;
  return CONCAT44(param_2,param_1);
}

