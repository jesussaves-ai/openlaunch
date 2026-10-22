// Function: FUN_00026f4c
// Address: 00026f4c


void FUN_00026f4c(void)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar2 = DAT_000278a8;
  *(uint *)(DAT_000278ac + 0x34) =
       *(ushort *)(DAT_000278a8 + 0xa8) & 0x8000 | *(ushort *)(DAT_000278a8 + 0xb6) & 0xffff7fff;
  iVar3 = FUN_00026cac();
  if (iVar3 != 0) {
    if (*DAT_000278b0 == 0) {
      iVar4 = FUN_000264e4(DAT_000278c0);
      iVar3 = DAT_000278c4;
      if (iVar4 != 0) {
        if (*(short *)(DAT_000278c4 + 8) == 0) {
          *(ushort *)(DAT_000278c4 + 8) = (ushort)*DAT_000278c8;
        }
        if ((*(ushort *)(iVar3 + 0xc) & 0x4000) != 0) {
          uVar1 = *(ushort *)(iVar2 + 0x44);
          *(ushort *)(iVar3 + 0x24) = uVar1 ^ uVar1 << 7 ^ uVar1 << 0xf;
          *(ushort *)(iVar3 + 0x26) =
               (*(ushort *)(iVar2 + 0x44) ^ *(ushort *)(iVar2 + 0x44) >> 7) & 0xff |
               (ushort)*DAT_000278cc << 0xe;
        }
        *(short *)(iVar2 + 0x2c) = (short)DAT_000278d0;
        *(undefined2 *)(iVar2 + 0xa8) = 0x8000;
        *(undefined2 *)(iVar2 + 0xae) = 0xd;
      }
    }
    else {
      FUN_00026c48(DAT_000278b4);
      *DAT_000278b8 = 0;
      *DAT_000278bc = 0;
    }
  }
  return;
}

