// Function: FUN_0602c374
// Address:  0602c374
// Signature: undefined FUN_0602c374()


ulonglong FUN_0602c374(void)

{
  uint uVar1;
  undefined4 in_cr0;
  undefined4 in_cr1;
  undefined4 in_cr2;
  undefined4 in_cr3;
  undefined4 in_cr4;
  undefined4 in_cr5;
  undefined4 in_cr6;
  undefined4 in_cr7;
  undefined4 in_cr9;
  
  uVar1 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar1 & 0xfffffffe);
  coproc_moveto_Translation_table_base_0(DAT_0602c424);
  coproc_moveto_Translation_table_base_1(DAT_0602c424);
  coproc_moveto_Domain_Access_Control(DAT_0602c428);
  coprocessor_moveto(0xf,0,2,DAT_0602c42c,in_cr5,in_cr0);
  coprocessor_moveto(0xf,0,3,DAT_0602c430,in_cr5,in_cr0);
  coprocessor_moveto(0xf,0,0,DAT_0602c434,in_cr6,in_cr0);
  coprocessor_moveto(0xf,0,0,DAT_0602c438,in_cr6,in_cr1);
  coprocessor_moveto(0xf,0,0,DAT_0602c43c,in_cr6,in_cr3);
  coprocessor_moveto(0xf,0,0,DAT_0602c440,in_cr6,in_cr4);
  coprocessor_moveto(0xf,0,0,DAT_0602c444,in_cr6,in_cr6);
  coprocessor_moveto(0xf,0,0,DAT_0602c448,in_cr6,in_cr7);
  coprocessor_moveto(0xf,0,0,DAT_0602c44c,in_cr9,in_cr1);
  coprocessor_moveto(0xf,0,1,DAT_0602c450,in_cr9,in_cr1);
  coprocessor_moveto(0xf,0,0,0,in_cr6,in_cr2);
  coprocessor_moveto(0xf,0,0,DAT_0602c4f4,in_cr6,in_cr5);
  coprocessor_moveto(0xf,0,0,DAT_0602c4f8,in_cr6,in_cr2);
  coprocessor_movefromRt(0xf,0,0,in_cr9,in_cr1);
  coproc_moveto_Invalidate_Entire_Instruction(0);
  coproc_moveto_Data_Synchronization(0);
  uVar1 = coproc_movefrom_Control();
  coproc_moveto_Control(uVar1 | 0x41005);
  return CONCAT44(DAT_0602c428,uVar1) | 0x41005;
}

