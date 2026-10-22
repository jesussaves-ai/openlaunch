// Function: FUN_0001cae0
// Address: 0001cae0


undefined4 FUN_0001cae0(void)

{
  uint uVar1;
  uint uVar2;
  
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

