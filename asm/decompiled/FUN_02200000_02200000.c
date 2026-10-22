// Function: FUN_02200000
// Address: 02200000


void FUN_02200000(void)

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
  coproc_moveto_Control(uRam022008bc);
  do {
  } while ((uRam04000180 & 0xf) != 0xe);
  uRam04000180 = 0x4e00;
  *puRam022008c0 = uRam022008c4;
  uRam04000240 = 0x80848981;
  uRam04000244 = 0x3000000;
  uRam04000248 = 0;
  *puRam022008c8 = 0xff;
  software_interrupt(0x110000);
  puVar1 = puRam022008d4;
  puVar3 = puRam022008d8;
  iVar5 = iRam022008dc;
  do {
    *puVar3 = *puVar1;
    iVar5 = iVar5 + -4;
    puVar2 = puRam022008e0;
    puVar1 = puVar1 + 1;
    puVar4 = puRam022008e4;
    puVar3 = puVar3 + 1;
    iVar6 = iRam022008e8;
  } while (iVar5 != 0);
  do {
    *puVar4 = *puVar2;
    iVar6 = iVar6 + -4;
    puVar2 = puVar2 + 1;
    puVar4 = puVar4 + 1;
  } while (iVar6 != 0);
                    /* WARNING: Could not recover jumptable at 0x022008b8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*pcRam022008ec)();
  return;
}

