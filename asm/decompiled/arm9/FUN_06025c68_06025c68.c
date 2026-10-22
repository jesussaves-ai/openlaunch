// Function: FUN_06025c68
// Address:  06025c68
// Signature: undefined FUN_06025c68()


void FUN_06025c68(undefined4 param_1,int param_2)

{
  byte *pbVar1;
  int iVar2;
  short *psVar3;
  
  pbVar1 = (byte *)(param_2 + 0xd);
  if ((*(byte *)(param_2 + 10) & 0x80) != 0) {
    iVar2 = 2 << (*(byte *)(param_2 + 10) & 7);
    psVar3 = DAT_06025d0c;
    do {
      *psVar3 = (ushort)(*pbVar1 >> 3) + (ushort)(pbVar1[1] >> 3) * 0x20 +
                (ushort)(pbVar1[2] >> 3) * 0x400;
      pbVar1 = pbVar1 + 3;
      psVar3 = psVar3 + 1;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}

