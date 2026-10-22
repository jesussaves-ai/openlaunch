// Function: FUN_00024ce0
// Address: 00024ce0


undefined4 FUN_00024ce0(void)

{
  *DAT_00025230 = 1;
  *DAT_00025234 = *DAT_00025234 & 0xfffe;
  *DAT_00025238 = *DAT_00025238 & 0xffffffef;
  *DAT_0002523c = 0xffffffff;
  return 0;
}

