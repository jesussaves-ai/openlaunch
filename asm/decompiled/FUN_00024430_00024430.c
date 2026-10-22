// Function: FUN_00024430
// Address: 00024430


undefined4 FUN_00024430(int param_1,undefined4 param_2,int param_3)

{
  uint uVar1;
  undefined4 uVar2;
  int *piVar3;
  int iVar4;
  bool bVar5;
  
  if (param_1 != 0) {
    iVar4 = *(int *)(param_1 + 0xc);
    uVar1 = FUN_00024408();
    bVar5 = uVar1 != DAT_00024878;
    if (bVar5) {
      uVar1 = (uint)*(ushort *)(iVar4 + 6);
    }
    if (bVar5 && uVar1 != 0) {
      FUN_000223c8(param_1);
    }
    else {
      uVar2 = FUN_00020ae4();
      piVar3 = DAT_0002487c;
      do {
        piVar3 = (int *)*piVar3;
        if (piVar3 == (int *)0x0) {
          FUN_000223c8(param_1);
          FUN_00020af8(uVar2);
          return 0;
        }
      } while (((piVar3[2] != 0 && piVar3[2] != param_3) || (piVar3[1] == 2)) ||
              (*(short *)(piVar3 + 4) != *(short *)(iVar4 + 2)));
      FUN_00022408(param_1,4);
      **(undefined4 **)(param_1 + 0xc) = param_2;
      if (piVar3[5] == 0) {
        piVar3[5] = param_1;
      }
      else {
        *(int *)(piVar3[6] + 8) = param_1;
      }
      do {
        iVar4 = param_1;
        param_1 = *(int *)(iVar4 + 8);
      } while (param_1 != 0);
      piVar3[6] = iVar4;
      FUN_00020af8(uVar2);
    }
  }
  return 0;
}

