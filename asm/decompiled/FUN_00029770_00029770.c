// Function: FUN_00029770
// Address: 00029770


void FUN_00029770(void)

{
  short sVar1;
  short *psVar2;
  short *psVar3;
  uint uVar4;
  
  FUN_0001c7d0(DAT_00029c24,DAT_00029c28,0x20);
  FUN_0001c7d0(DAT_00029c2c,DAT_00029c30,0x200);
  sVar1 = -0x8000;
  uVar4 = 0;
  psVar2 = DAT_00029c34;
  do {
    psVar3 = psVar2 + 1;
    *psVar2 = sVar1;
    sVar1 = sVar1 + 1;
    uVar4 = uVar4 + 1;
    if ((uVar4 & 3) == 0) {
      psVar3 = psVar2 + 0x1d;
    }
    psVar2 = psVar3;
  } while ((uVar4 & 0xf) != 0);
  return;
}

