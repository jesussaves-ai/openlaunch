// Function: FUN_06027370
// Address:  06027370
// Signature: undefined FUN_06027370()


undefined8 FUN_06027370(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 extraout_r1;
  undefined4 *extraout_r1_00;
  undefined4 *puVar2;
  int unaff_r4;
  undefined8 uVar3;
  
  FUN_0602c1a8(unaff_r4,DAT_06027560,0x10);
  if (((*(byte *)(unaff_r4 + 0x1c) & 4) == 0) && ((*(byte *)(unaff_r4 + 0x1bf) & 0x80) == 0)) {
    FUN_0602c1a8(DAT_06027564,extraout_r1,8);
    uVar1 = *(undefined4 *)(unaff_r4 + 0xc);
    *extraout_r1_00 = uVar1;
    uVar3 = FUN_0602bf6c(uVar1,extraout_r1_00 + 1);
    puVar2 = (undefined4 *)((ulonglong)uVar3 >> 0x20);
    *puVar2 = (int)uVar3;
    FUN_0602c1a8(unaff_r4 + 0x350,puVar2 + 1,0x10);
  }
  return CONCAT44(param_2,param_1);
}

