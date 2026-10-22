// Function: FUN_06029024
// Address:  06029024
// Signature: undefined FUN_06029024()


/* WARNING: Removing unreachable block (ram,0x06029150) */
/* WARNING: Removing unreachable block (ram,0x060291c0) */
/* WARNING: Removing unreachable block (ram,0x060291c4) */
/* WARNING: Removing unreachable block (ram,0x060291cc) */
/* WARNING: Removing unreachable block (ram,0x060291d0) */

void FUN_06029024(void)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  ushort *puVar5;
  
  puVar5 = (ushort *)(DAT_060297a8 + (uint)DAT_06028515 * 0x10);
  iVar2 = (uint)*puVar5 + (uint)puVar5[1] * 0x10000;
  uVar3 = 0;
  uVar4 = DAT_060297ac;
  if (iVar2 != 0) {
    FUN_06029f00(iVar2,DAT_060297b0);
    uVar3 = (uint)*(ushort *)(DAT_060297b4 + 0x1fe);
    uVar4 = DAT_060297bc;
    if (uVar3 == DAT_060297b8) {
      do {
      } while (*(char *)(DAT_060297c0 + 0x15) != -8);
      DAT_06028760 = (uint)*(ushort *)(DAT_060297c4 + 0x11);
      DAT_06028770 = (uint)*(byte *)(DAT_060297cc + 0xd);
      DAT_06028768 = (uint)*(ushort *)(DAT_060297d0 + 0xe);
      DAT_06028774 = (uint)*(byte *)(DAT_060297d4 + 0x10);
      DAT_0602876c = (uint)*(ushort *)(DAT_060297d8 + 0x16);
      DAT_06028794 = 1;
      if (DAT_0602876c == 0) {
        DAT_0602876c = *(uint *)(DAT_060297d8 + 0x24);
        DAT_06028794 = *(undefined4 *)(DAT_060297d8 + 0x2c);
      }
      DAT_06028778 = *(undefined4 *)(DAT_060297dc + 0x1c);
      DAT_0602877c = *(undefined4 *)(DAT_060297e0 + 0x20);
      DAT_06028798 = DAT_06028760 * 0x20;
      software_interrupt(0x90000);
      do {
      } while (*(short *)(DAT_060297c8 + 0xb) != 0);
      uVar3 = 0;
      DAT_06028764 = 0;
      uVar4 = DAT_060297e8;
      DAT_06028790 = uVar3;
      DAT_0602879c = DAT_06028798;
    }
  }
  uVar1 = FUN_060264a0(uVar3,DAT_060250c8);
  FUN_060264a0(uVar1,uVar4);
  FUN_06026f8c(0,0x12);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}

