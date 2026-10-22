// Function: FUN_060288d4
// Address:  060288d4
// Signature: undefined FUN_060288d4()


undefined8 FUN_060288d4(uint param_1,undefined4 param_2)

{
  byte bVar1;
  int iVar2;
  byte *pbVar3;
  undefined1 uVar4;
  
  if (*DAT_06029674 != param_1) {
    *DAT_06029674 = (byte)param_1;
    pbVar3 = (byte *)(DAT_06029678 + (param_1 - 0x41) * 2);
    bVar1 = *pbVar3;
    DAT_06028515 = pbVar3[1];
    uVar4 = bVar1 == 0;
    iVar2 = DAT_0602967c;
    if (!(bool)uVar4) {
      iVar2 = DAT_06029680;
    }
    *DAT_06029684 = iVar2;
    *(ushort *)(iVar2 + 0x42) = (ushort)bVar1;
    *(ushort *)(iVar2 + 0x2e) = (ushort)bVar1;
    *(undefined2 *)(iVar2 + 0x32) = 0;
    *(undefined4 *)(iVar2 + 0x34) = 0;
    *(undefined2 *)(iVar2 + 0x40) = 0;
    FUN_06028960();
    FUN_06029e38();
    if ((bool)uVar4) {
      FUN_06029e5c();
      FUN_06028cdc();
      if ((bool)uVar4) {
        FUN_06028ff8();
        FUN_06029024();
      }
    }
  }
  return CONCAT44(param_2,param_1);
}

