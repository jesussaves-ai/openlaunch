// Function: FUN_06024540
// Address:  06024540
// Signature: undefined FUN_06024540()


void FUN_06024540(void)

{
  byte bVar1;
  int iVar2;
  byte bVar3;
  byte *pbVar4;
  uint uVar5;
  uint uVar6;
  ushort *puVar7;
  
  iVar2 = DAT_060250f0;
  if (*(char *)(DAT_060250f0 + 0x12) != '\0') {
    FUN_0602c104(DAT_060250f4,0x400);
    uVar6 = 0;
    FUN_0602a72c(5);
    for (puVar7 = (ushort *)*DAT_060250f8; puVar7 < (ushort *)*DAT_060250fc;
        puVar7 = (ushort *)((int)puVar7 + (uint)*puVar7)) {
      if ((*(byte *)((int)puVar7 + 3) & 0x10) == 0) {
        if (0x75 < uVar6) break;
        FUN_0602aa84(puVar7);
        if (((DAT_06024688 != *(uint *)(puVar7 + 0xe) >> 8 &&
              DAT_0602468c != *(uint *)(puVar7 + 0xe) >> 8) &&
            (*(int *)(puVar7 + 0xe) != 0 || *(int *)(puVar7 + 0x10) != 0)) && (puVar7[0xc] != 0)) {
          uVar5 = (uint)(*(int *)(puVar7 + 0x12) != 0);
          if (*(int *)(puVar7 + 0x14) != 0) {
            uVar5 = uVar5 | 2;
          }
          if ((*(byte *)((int)puVar7 + 0x1b) & 1) != 0) {
            uVar5 = uVar5 | 4;
          }
          if (*(int *)(puVar7 + 0xe) == *(int *)(iVar2 + 0x230) &&
              *(int *)(puVar7 + 0x10) == *(int *)(iVar2 + 0x234)) {
            uVar5 = uVar5 | 4;
          }
          if (puVar7[0xc] == *(ushort *)(iVar2 + 0x10)) {
            uVar5 = uVar5 | 8;
          }
          else {
            uVar5 = uVar5 & 0xfffffffc;
          }
          if ((uVar5 & 0xc) != 0) {
            pbVar4 = (byte *)(DAT_06025100 + (uVar6 >> 3));
            do {
              bVar3 = *pbVar4;
              bVar1 = (byte)uVar5;
              uVar5 = uVar5 >> 1;
              if ((bool)(bVar1 & 1)) {
                bVar3 = bVar3 | (byte)(1 << (uVar6 & 7));
              }
              *pbVar4 = bVar3;
              pbVar4 = pbVar4 + 0x10;
            } while (uVar5 != 0);
            FUN_0602c1a8(puVar7 + 0xe,DAT_06025104 + uVar6 * 8,8);
            uVar6 = uVar6 + 1;
          }
        }
      }
    }
    *DAT_06025108 = uVar6;
  }
  return;
}

