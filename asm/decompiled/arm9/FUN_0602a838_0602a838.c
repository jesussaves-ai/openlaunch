// Function: FUN_0602a838
// Address:  0602a838
// Signature: undefined FUN_0602a838()


void FUN_0602a838(void)

{
  uint *puVar1;
  undefined4 uVar2;
  int extraout_r1;
  uint uVar3;
  short *psVar4;
  
  uVar2 = DAT_0602adac;
  puVar1 = DAT_0602ad94;
  psVar4 = (short *)*DAT_0602ad94;
  *(undefined1 *)(psVar4 + 1) = 0;
  *(undefined1 *)((int)psVar4 + 3) = 0;
  psVar4[2] = -1;
  psVar4[3] = -1;
  psVar4[4] = 0;
  psVar4[5] = 0;
  FUN_0602a4e0(uVar2,psVar4 + 0x1a);
  uVar3 = extraout_r1 + 3U & 0xfffffffc;
  *puVar1 = uVar3;
  *psVar4 = (short)uVar3 - (short)psVar4;
  return;
}

