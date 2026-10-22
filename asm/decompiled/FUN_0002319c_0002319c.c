// Function: FUN_0002319c
// Address: 0002319c


int FUN_0002319c(int param_1,byte param_2,int param_3)

{
  undefined2 uVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  undefined2 *puVar6;
  
  iVar2 = FUN_00022234();
  iVar2 = FUN_00022378(iVar2 + 0x14 + param_3);
  if (iVar2 != 0) {
    iVar3 = FUN_00022234();
    FUN_00022408(iVar2,-iVar3);
    puVar6 = *(undefined2 **)(iVar2 + 0xc);
    uVar4 = FUN_00022088(*(undefined4 *)(param_1 + 8));
    *(undefined4 *)(puVar6 + 2) = uVar4;
    uVar4 = FUN_00022088(*(undefined4 *)(param_1 + 0xc));
    *(undefined4 *)(puVar6 + 4) = uVar4;
    *puVar6 = *(undefined2 *)(param_1 + 0x30);
    puVar6[1] = *(undefined2 *)(param_1 + 0x32);
    *(byte *)((int)puVar6 + 0xd) = param_2;
    puVar6[9] = 0;
    puVar6[8] = 0;
    *(undefined1 *)(puVar6 + 6) = 0x50;
    iVar3 = *(uint *)(param_1 + 0x4c) - *(uint *)(param_1 + 0x48);
    if (*(uint *)(param_1 + 0x4c) < *(uint *)(param_1 + 0x48)) {
      iVar3 = iVar3 + 0x2000;
    }
    uVar5 = DAT_00023a8c - iVar3;
    if ((param_2 & 0x10) != 0) {
      *(uint *)(param_1 + 0x44) = (uint)(uVar5 < 1000);
    }
    if (DAT_00023a90 < uVar5) {
      uVar5 = DAT_00023a90;
    }
    *(uint *)(param_1 + 0x14) = *(int *)(param_1 + 0xc) + uVar5;
    uVar1 = FUN_00022078();
    puVar6[7] = uVar1;
  }
  return iVar2;
}

