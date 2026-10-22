// Function: FUN_06025d14
// Address:  06025d14
// Signature: undefined FUN_06025d14()


undefined8 FUN_06025d14(undefined4 param_1,byte *param_2)

{
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  byte *pbVar4;
  int iVar5;
  byte *pbVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  undefined2 *puVar11;
  uint uVar12;
  uint uVar13;
  
  iVar2 = DAT_06025df0;
  pbVar6 = param_2 + 1;
  uVar7 = (uint)*param_2;
  uVar8 = 0;
  uVar9 = 0;
  puVar11 = DAT_06025d08;
  do {
    uVar13 = DAT_06025d10 + 1;
    uVar1 = 1 << (DAT_06025d10 & 0xff);
    uVar12 = uVar1 + 1;
    while( true ) {
      if ((uVar12 >> (uVar13 & 0xff) != 0) && (uVar13 != 0xc)) {
        uVar13 = uVar13 + 1;
      }
      uVar12 = uVar12 + 1;
      *(undefined2 **)(iVar2 + uVar12 * 4) = puVar11;
      pbVar4 = pbVar6;
      uVar10 = uVar9;
      while (pbVar6 = pbVar4, uVar8 < uVar13) {
        uVar10 = uVar10 | (uint)*pbVar6 << (uVar8 & 0xff);
        uVar8 = uVar8 + 8;
        uVar7 = uVar7 - 1;
        pbVar4 = pbVar6 + 1;
        if (uVar7 == 0) {
          uVar7 = (uint)pbVar6[1];
          pbVar4 = pbVar6 + 2;
        }
      }
      uVar9 = uVar10 >> (uVar13 & 0xff);
      uVar10 = uVar10 ^ uVar9 << (uVar13 & 0xff);
      uVar8 = uVar8 - uVar13;
      pbVar4 = (byte *)(uVar10 - uVar1);
      if (uVar10 < uVar1) {
        uVar10 = uVar10 + 0x10;
        iVar5 = -1;
        goto LAB_06025db0;
      }
      if (pbVar4 == (byte *)0x0) break;
      if (pbVar4 < (byte *)0x2) {
        return CONCAT44(param_2,param_1);
      }
      puVar3 = (undefined4 *)(iVar2 + uVar10 * 4);
      pbVar4 = (byte *)*puVar3;
      iVar5 = puVar3[1] - (int)pbVar4;
      do {
        uVar10 = (uint)*pbVar4;
        pbVar4 = pbVar4 + 1;
LAB_06025db0:
        if (((uint)puVar11 & 1) == 0) {
          *puVar11 = (short)uVar10;
        }
        else {
          *(ushort *)((int)puVar11 + -1) =
               (ushort)*(byte *)((int)puVar11 + -1) | (ushort)(uVar10 << 8);
        }
        puVar11 = (undefined2 *)((int)puVar11 + 1);
        iVar5 = iVar5 + -1;
      } while (-1 < iVar5);
    }
  } while( true );
}

