// Function: Reset
// Address: 00000000


void Reset(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  int iVar5;
  int iVar6;
  
  coproc_moveto_Data_Synchronization(0);
  coproc_moveto_Invalidate_Entire_Instruction(0);
  coproc_moveto_Invalidate_Entire_Data_cache(0);
  coproc_moveto_Control(uRam000008bc);
  do {
  } while ((uRam04000180 & 0xf) != 0xe);
  uRam04000180 = 0x4e00;
  *puRam000008c0 = uRam000008c4;
  uRam04000240 = 0x80848981;
  uRam04000244 = 0x3000000;
  uRam04000248 = 0;
  *puRam000008c8 = 0xff;
  software_interrupt(0x110000);
  puVar1 = puRam000008d4;
  puVar3 = puRam000008d8;
  iVar5 = iRam000008dc;
  do {
    *puVar3 = *puVar1;
    iVar5 = iVar5 + -4;
    puVar2 = puRam000008e0;
    puVar1 = puVar1 + 1;
    puVar4 = puRam000008e4;
    puVar3 = puVar3 + 1;
    iVar6 = iRam000008e8;
  } while (iVar5 != 0);
  do {
    *puVar4 = *puVar2;
    iVar6 = iVar6 + -4;
    puVar2 = puVar2 + 1;
    puVar4 = puVar4 + 1;
  } while (iVar6 != 0);
                    /* WARNING: Could not recover jumptable at 0x000008b8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*pcRam000008ec)();
  return;
}

