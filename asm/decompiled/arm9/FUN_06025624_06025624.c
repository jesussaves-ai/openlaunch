// Function: FUN_06025624
// Address:  06025624
// Signature: undefined FUN_06025624()


void FUN_06025624(void)

{
  int iVar1;
  byte *pbVar2;
  undefined4 *puVar3;
  
  pbVar2 = DAT_06025708;
  iVar1 = DAT_06025704;
  if ((*(uint *)(DAT_06025708 + 8) & 0x80000000) != 0) {
    *(undefined4 *)(DAT_06025704 + 4) = *(undefined4 *)(DAT_06025708 + 8);
    *(byte *)(iVar1 + 8) =
         pbVar2[0x24] | (*pbVar2 & 3) << 2 | (pbVar2[1] & 6) << 4 | pbVar2[0x20] << 7;
    *(byte *)(iVar1 + 9) =
         (byte)*(ushort *)(pbVar2 + 6) | (byte)((*(ushort *)(pbVar2 + 6) & 0x100) >> 6) |
         (byte)((*(ushort *)(pbVar2 + 4) & 7) << 3) | (byte)((*(ushort *)(pbVar2 + 4) & 0x180) >> 1)
    ;
  }
  puVar3 = DAT_0602570c;
  *DAT_0602570c = *(undefined4 *)(iVar1 + 4);
  *(undefined2 *)(puVar3 + 1) = *(undefined2 *)(iVar1 + 8);
  return;
}

