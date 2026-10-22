// Function: FUN_0002452c
// Address: 0002452c


int FUN_0002452c(int param_1,int param_2,int param_3,undefined4 param_4)

{
  undefined2 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined2 unaff_r4;
  undefined2 *puVar6;
  
  if ((param_2 == 0) || (param_1 == 0)) {
    param_3 = FUN_0001cd28();
  }
  else {
    if (*(int *)(param_1 + 4) != 1) {
      *(undefined4 *)(param_1 + 8) = 0;
      uVar1 = FUN_000243bc();
      *(undefined2 *)(param_1 + 0x10) = uVar1;
      *(undefined4 *)(param_1 + 4) = 1;
    }
    iVar2 = FUN_00022234();
    puVar3 = (undefined4 *)FUN_00022378(iVar2 + 8 + param_3);
    if (puVar3 == (undefined4 *)0x0) {
      param_3 = FUN_0001cd38();
    }
    else {
      iVar2 = FUN_00022234();
      FUN_00022408(puVar3,-iVar2);
      uVar4 = FUN_00020ae4();
      uVar5 = FUN_000222fc(*(undefined4 *)(param_1 + 8),param_4);
      puVar6 = (undefined2 *)puVar3[3];
      *puVar6 = *(undefined2 *)(param_1 + 0x10);
      puVar6[1] = unaff_r4;
      uVar1 = FUN_00022078(param_3 + 8);
      puVar6[2] = uVar1;
      puVar6[3] = 0;
      FUN_0001c7d0(param_2,puVar6 + 4,param_3);
      uVar1 = FUN_00024408(puVar3,uVar5,param_4,*puVar3);
      puVar6[3] = uVar1;
      FUN_0002223c(puVar3,0x11,uVar5,param_4);
      FUN_00020af8(uVar4);
    }
  }
  return param_3;
}

