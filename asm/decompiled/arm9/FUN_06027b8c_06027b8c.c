// Function: FUN_06027b8c
// Address:  06027b8c
// Signature: undefined FUN_06027b8c()


void FUN_06027b8c(void)

{
  uint *puVar1;
  undefined4 uVar2;
  undefined2 uVar3;
  undefined4 uVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  
  uVar2 = DAT_06028324;
  uVar7 = (uint)*(byte *)(DAT_06028320 + 0xa2);
  piVar5 = (int *)(DAT_06028320 + 0xa4);
  do {
    puVar1 = (uint *)(piVar5 + 2);
    if (*puVar1 == (uint)*DAT_0602831c) {
      iVar6 = *piVar5 + DAT_06028320;
      *DAT_06028328 = iVar6;
      FUN_0602c1a8(iVar6 + (uint)*(ushort *)(iVar6 + 2) + (uint)*(byte *)(iVar6 + 1) * 8 + 4,
                   DAT_0602832c,0xc);
      uVar3 = FUN_0602bf64(DAT_06028330,DAT_06028334,0xc);
      iVar6 = DAT_06028338;
      *(undefined2 *)(DAT_06028338 + 2) = uVar3;
      *(undefined1 *)(iVar6 + 1) = 0;
      *(undefined4 *)(iVar6 + 0x10) = 0;
      *(undefined4 *)(iVar6 + 0x14) = 0;
      *(undefined4 *)(iVar6 + 0x18) = 0;
      *(undefined4 *)(iVar6 + 0x1c) = 0;
      return;
    }
    piVar5 = piVar5 + 8;
    uVar7 = uVar7 - 1;
  } while (uVar7 != 0);
  uVar4 = FUN_060264a0(*puVar1,DAT_060250c8);
  FUN_060264a0(uVar4,uVar2);
  FUN_06026f8c(0,0x12);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}

