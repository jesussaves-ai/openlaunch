// Function: FUN_0002223c
// Address: 0002223c


void FUN_0002223c(undefined4 *param_1,undefined1 param_2,undefined4 param_3,undefined4 param_4)

{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  ushort *puVar5;
  ushort *puVar6;
  bool bVar7;
  
  FUN_00022408(param_1,0x14);
  puVar6 = (ushort *)param_1[3];
  *(undefined1 *)((int)puVar6 + 9) = param_2;
  *(undefined4 *)(puVar6 + 6) = param_3;
  *(undefined4 *)(puVar6 + 8) = param_4;
  puVar6[3] = 0;
  *(undefined1 *)((int)puVar6 + 1) = 0;
  iVar2 = *DAT_0002236c;
  *DAT_0002236c = iVar2 + 1;
  puVar6[2] = (ushort)(iVar2 + 1);
  uVar1 = FUN_00022078(*param_1);
  puVar6[1] = uVar1;
  *(undefined1 *)(puVar6 + 4) = 0x80;
  *(undefined1 *)puVar6 = 0x45;
  uVar3 = 0;
  puVar6[5] = 0;
  iVar2 = 10;
  puVar5 = puVar6;
  do {
    uVar3 = uVar3 + *puVar5;
    iVar2 = iVar2 + -1;
    puVar5 = puVar5 + 1;
  } while (iVar2 != 0);
  bVar7 = CARRY4(uVar3,uVar3 * 0x10000);
  uVar3 = uVar3 * 0x10001;
  uVar4 = uVar3;
  if (bVar7) {
    uVar4 = uVar3 + 0x10000;
  }
  if (bVar7 && 0xfffeffff < uVar3) {
    uVar4 = uVar4 + 0x10000;
  }
  uVar1 = (ushort)(~uVar4 >> 0x10);
  if (~uVar4 >> 0x10 == 0) {
    uVar1 = (ushort)DAT_00022370;
  }
  puVar6[5] = uVar1;
  FUN_00021f40(8,param_1,param_4,param_3);
  return;
}

