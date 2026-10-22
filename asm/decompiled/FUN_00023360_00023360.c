// Function: FUN_00023360
// Address: 00023360


undefined4 FUN_00023360(undefined1 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined2 uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined2 unaff_r5;
  undefined2 unaff_r6;
  uint unaff_r7;
  undefined2 *puVar6;
  
  uVar2 = FUN_00020ae4();
  iVar3 = FUN_00022234();
  iVar3 = FUN_00022378(iVar3 + 0x14);
  if (iVar3 != 0) {
    iVar4 = FUN_00022234();
    FUN_00022408(iVar3,-iVar4);
    puVar6 = *(undefined2 **)(iVar3 + 0xc);
    *(undefined1 *)((int)puVar6 + 0xd) = param_1;
    *puVar6 = unaff_r5;
    puVar6[1] = unaff_r6;
    puVar6[9] = 0;
    puVar6[8] = 0;
    *(undefined1 *)(puVar6 + 6) = 0x50;
    if (DAT_00023a9c < unaff_r7) {
      unaff_r7 = DAT_00023a9c;
    }
    uVar1 = FUN_00022078(unaff_r7);
    puVar6[7] = uVar1;
    uVar5 = FUN_00022088(param_2);
    *(undefined4 *)(puVar6 + 2) = uVar5;
    uVar5 = FUN_00022088(param_3);
    *(undefined4 *)(puVar6 + 4) = uVar5;
    FUN_00023164(iVar3,param_4);
    FUN_0002223c(iVar3,6,param_4);
  }
  FUN_00020af8(uVar2);
  return 0;
}

