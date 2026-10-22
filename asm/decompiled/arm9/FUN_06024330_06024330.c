// Function: FUN_06024330
// Address:  06024330
// Signature: undefined FUN_06024330()


undefined8 FUN_06024330(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined *puVar4;
  
  iVar2 = *DAT_060250a8;
  puVar4 = DAT_060250ac;
  if (iVar2 == 1) {
    iVar2 = FUN_06026fc0(1,0x20);
    iVar1 = DAT_060250b4;
    puVar4 = DAT_060250b0;
    if (3 < iVar2) {
      *(undefined4 *)(DAT_060250b4 + 4) = 0x5a0;
      iVar2 = *(int *)(iVar1 + 4);
      puVar4 = PTR_s_Requires_DSiware___SD_MMC_access_060250b8;
      if (iVar2 == 0x5a0) {
        return CONCAT44(param_2,param_1);
      }
    }
  }
  uVar3 = FUN_060264a0(iVar2,DAT_060250c8);
  FUN_060264a0(uVar3,puVar4);
  FUN_06026f8c(0,0x12);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}

