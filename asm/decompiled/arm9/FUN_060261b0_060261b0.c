// Function: FUN_060261b0
// Address:  060261b0
// Signature: undefined FUN_060261b0()


void FUN_060261b0(void)

{
  byte bVar1;
  byte *pbVar2;
  int *piVar3;
  int *piVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  
  piVar4 = DAT_06026730;
  do {
    uVar5 = 0x300;
    pbVar2 = DAT_06026734;
    piVar3 = piVar4;
    do {
      bVar1 = *pbVar2 | *pbVar2 >> 1;
      uVar6 = (uint)bVar1;
      uVar9 = uVar6;
      if ((uVar5 & 7) != 0) {
        uVar9 = (uint)(byte)(bVar1 | pbVar2[-1] | pbVar2[-1] >> 1);
      }
      if ((uVar5 & 7) != 1) {
        bVar1 = pbVar2[1];
        uVar9 = uVar9 | (byte)(bVar1 | bVar1 >> 1);
      }
      uVar9 = uVar9 | uVar9 >> 1;
      uVar9 = uVar9 | uVar9 << 1;
      uVar7 = 0x10000000;
      iVar8 = 0;
      do {
        bVar1 = (byte)uVar6;
        uVar6 = uVar6 >> 1;
        if ((bool)(bVar1 & 1)) {
          iVar8 = iVar8 + uVar7;
        }
        else if ((uVar9 & 1) != 0) {
          iVar8 = iVar8 + uVar7 * 2;
        }
        uVar9 = uVar9 >> 1;
        uVar7 = uVar7 >> 4;
      } while (uVar7 != 0);
      *piVar3 = iVar8;
      uVar5 = uVar5 - 1;
      pbVar2 = pbVar2 + 1;
      piVar3 = piVar3 + 1;
    } while (uVar5 != 0);
    piVar4 = (int *)((uint)piVar4 ^ 0x200000);
  } while (((uint)piVar4 & 0x200000) != 0);
  return;
}

