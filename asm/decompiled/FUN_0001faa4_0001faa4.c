// Function: FUN_0001faa4
// Address: 0001faa4


void FUN_0001faa4(byte *param_1,byte *param_2,int param_3,int param_4)

{
  byte bVar1;
  undefined1 uVar2;
  char cVar3;
  uint *puVar4;
  uint uVar5;
  int iVar6;
  int unaff_r4;
  int unaff_r5;
  uint uVar7;
  byte *pbVar8;
  bool bVar9;
  uint local_128 [64];
  
  param_3 = param_3 + unaff_r5;
  puVar4 = local_128;
  uVar5 = DAT_000205dc;
  do {
    *puVar4 = uVar5;
    bVar9 = CARRY4(uVar5,DAT_000205e0);
    uVar5 = uVar5 + DAT_000205e0;
    puVar4 = puVar4 + 1;
  } while (!bVar9);
  uVar5 = 0;
  uVar7 = 0;
  iVar6 = 0;
  do {
    bVar1 = *(byte *)((int)local_128 + (uVar5 >> 0x18));
    uVar7 = uVar7 + (uint)*(byte *)(param_4 + iVar6) * 0x1000000 + (uint)bVar1 * 0x1000000;
    uVar2 = *(undefined1 *)((int)local_128 + (uVar7 >> 0x18));
    *(byte *)((int)local_128 + (uVar7 >> 0x18)) = bVar1;
    *(undefined1 *)((int)local_128 + (uVar5 >> 0x18)) = uVar2;
    iVar6 = iVar6 + 1;
    if (iVar6 == unaff_r4) {
      iVar6 = 0;
    }
    bVar9 = uVar5 < 0xff000000;
    uVar5 = uVar5 + 0x1000000;
  } while (bVar9);
  uVar7 = 0;
  do {
    uVar5 = uVar5 + 0x1000000;
    bVar1 = *(byte *)((int)local_128 + (uVar5 >> 0x18));
    uVar7 = uVar7 + (uint)bVar1 * 0x1000000;
    cVar3 = *(char *)((int)local_128 + (uVar7 >> 0x18));
    *(byte *)((int)local_128 + (uVar7 >> 0x18)) = bVar1;
    *(char *)((int)local_128 + (uVar5 >> 0x18)) = cVar3;
    unaff_r5 = unaff_r5 + -1;
    pbVar8 = param_2;
    if (unaff_r5 < 0) {
      pbVar8 = param_2 + 1;
      *param_2 = *param_1 ^ *(byte *)((int)local_128 + (uint)(byte)(bVar1 + cVar3));
      param_1 = param_1 + 1;
    }
    bVar9 = param_3 != 0;
    param_3 = param_3 + -1;
    param_2 = pbVar8;
  } while (bVar9 && param_3 != 0);
  return;
}

