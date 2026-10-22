// Function: FUN_000248c8
// Address: 000248c8


undefined4 * FUN_000248c8(int param_1)

{
  undefined4 uVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  uint uVar5;
  
  uVar1 = FUN_00020ae4();
  uVar5 = param_1 + 3U & 0xfffffffc;
  if (uVar5 == 0) {
    uVar5 = 4;
  }
  puVar3 = (undefined4 *)*DAT_00024a80;
  if ((undefined4 *)*DAT_00024a80 == (undefined4 *)0x0) {
LAB_000249f0:
    puVar3 = (undefined4 *)0x0;
  }
  else {
    while (uVar2 = puVar3[2] - uVar5, (uint)puVar3[2] < uVar5) {
      puVar4 = (undefined4 *)*puVar3;
      if (puVar4 == (undefined4 *)0x0) goto LAB_000249f0;
      if (*(short *)(puVar4 + 1) == 0) {
        do {
          puVar4 = (undefined4 *)*puVar4;
          if (puVar4 == (undefined4 *)0x0) goto LAB_000249f0;
          puVar3 = puVar4;
        } while (*(short *)(puVar4 + 1) == 0);
      }
      else {
        *puVar3 = *puVar4;
        puVar3[2] = puVar4[2] + puVar3[2] + 0xc;
      }
    }
    *(undefined2 *)(puVar3 + 1) = 0;
    if (uVar2 < 0x4c) {
      *(short *)((int)puVar3 + 6) = (short)uVar2;
    }
    else {
      puVar4 = (undefined4 *)((int)puVar3 + uVar5 + 0xc);
      puVar4[2] = uVar2 - 0xc;
      *(undefined2 *)(puVar4 + 1) = 1;
      *puVar4 = *puVar3;
      *puVar3 = puVar4;
      puVar3[2] = uVar5;
      *(undefined2 *)((int)puVar3 + 6) = 0;
    }
    puVar4 = (undefined4 *)*DAT_00024a84;
    if (puVar4 == puVar3) {
      do {
        puVar4 = (undefined4 *)*puVar4;
        if (puVar4 == (undefined4 *)0x0) break;
      } while (*(short *)(puVar4 + 1) == 0);
      *DAT_00024a84 = puVar4;
    }
    puVar3 = puVar3 + 3;
  }
  FUN_00020af8(uVar1);
  return puVar3;
}

