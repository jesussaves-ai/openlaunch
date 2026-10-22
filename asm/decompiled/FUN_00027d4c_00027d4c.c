// Function: FUN_00027d4c
// Address: 00027d4c


void FUN_00027d4c(void)

{
  int extraout_r1;
  undefined4 uStack_44;
  undefined4 uStack_40;
  uint uStack_3c;
  undefined2 uStack_38;
  ushort uStack_36;
  undefined1 auStack_34 [52];
  
  uStack_44 = 0;
  uStack_40 = 0;
  uStack_3c = (uint)*DAT_00028298;
  uStack_38 = 0x1a4;
  uStack_36 = *(ushort *)(DAT_0002829c + 0x2a) | 0xc000;
  FUN_00025f60(DAT_000282a0,auStack_34);
  FUN_00025f60(DAT_000282a4);
  FUN_000279fc(&uStack_44,extraout_r1 - (int)&uStack_44);
  return;
}

