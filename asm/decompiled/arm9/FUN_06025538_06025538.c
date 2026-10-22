// Function: FUN_06025538
// Address:  06025538
// Signature: undefined FUN_06025538()


undefined4 FUN_06025538(void)

{
  uint uVar1;
  uint uVar2;
  
  *DAT_060256d8 = (uint)((*DAT_060256d4 & 3) == 1);
  coproc_moveto_Data_Synchronization(0);
  uVar1 = 0;
  do {
    do {
      coproc_moveto_Clean_Data_Cache_by_Index(uVar1);
      uVar1 = uVar1 + 0x20;
    } while ((uVar1 & 0x400) == 0);
    uVar2 = uVar1 & 0xfffffbff;
    uVar1 = uVar2 + 0x40000000;
  } while (uVar2 < 0xc0000000);
  coproc_moveto_Data_Synchronization(0);
  coproc_moveto_Invalidate_Entire_Instruction(0);
  coproc_moveto_Invalidate_Entire_Data_cache(0);
  return 0;
}

