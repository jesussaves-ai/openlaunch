// Function: FUN_00026260
// Address: 00026260


byte FUN_00026260(undefined4 param_1,undefined4 param_2,uint param_3,int param_4)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  byte *pbVar4;
  byte *pbVar5;
  uint uVar6;
  uint uVar7;
  byte bVar8;
  undefined8 uVar9;
  undefined8 uVar10;
  
  uVar6 = param_3 - 2;
  if (((1 < param_3) && (iVar2 = FUN_0001c918(), iVar2 == 1)) && (uVar7 = uVar6 - 4, 3 < uVar6)) {
    uVar9 = FUN_0001c8fc();
    pcVar3 = (char *)((ulonglong)uVar9 >> 0x20);
    if ((int)uVar9 == param_4) {
      cVar1 = *pcVar3;
      bVar8 = cVar1 == '\x01';
      if (cVar1 == '\x02') {
        bVar8 = 4;
      }
      if (cVar1 == '\x04') {
        bVar8 = bVar8 | 8;
      }
      if (cVar1 == '\x05') {
        bVar8 = bVar8 | 2;
      }
      if ((bVar8 != 0) && (uVar6 = uVar7 - 2, 1 < uVar7)) {
        uVar9 = FUN_0001c918(cVar1,pcVar3 + 1);
        uVar7 = (uint)uVar9;
        if (uVar7 << 2 <= uVar6) {
          while( true ) {
            iVar2 = (int)uVar9 + -1;
            if ((int)uVar9 == 0) break;
            uVar10 = FUN_0001c8fc(uVar7,(int)((ulonglong)uVar9 >> 0x20));
            pbVar4 = (byte *)((ulonglong)uVar10 >> 0x20);
            pbVar5 = pbVar4 + 1;
            uVar7 = (uint)*pbVar4;
            uVar9 = CONCAT44(pbVar5,iVar2);
            if ((int)uVar10 == param_4) {
              if (uVar7 == 0) {
                bVar8 = bVar8 | 0x10;
              }
              if (uVar7 == 2) {
                bVar8 = bVar8 | 0x20;
              }
              uVar9 = CONCAT44(pbVar5,iVar2);
              if (uVar7 == 4) {
                bVar8 = bVar8 | 0x40;
                uVar9 = CONCAT44(pbVar5,iVar2);
              }
            }
          }
          return bVar8;
        }
      }
    }
  }
  return 0;
}

