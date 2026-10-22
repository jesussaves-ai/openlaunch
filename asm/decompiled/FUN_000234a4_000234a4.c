// Function: FUN_000234a4
// Address: 000234a4


void FUN_000234a4(int *param_1)

{
  undefined4 uVar1;
  int *piVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  
  if (param_1 != (int *)0x0) {
    uVar1 = FUN_00020ae4();
    param_1[1] = 0;
    piVar2 = DAT_00023aa8;
    do {
      piVar3 = piVar2;
      piVar2 = (int *)*piVar3;
    } while (piVar2 != param_1);
    *piVar3 = *param_1;
    if ((int *)param_1[0xd] != (int *)0x0) {
      iVar4 = param_1[0xe];
      piVar2 = (int *)param_1[0xd];
      do {
        if (*piVar2 == 0) break;
        FUN_000234a4();
        iVar4 = iVar4 + -1;
        piVar2 = piVar2 + 1;
      } while (iVar4 != 0);
      piVar2 = DAT_00023aac;
      iVar5 = *DAT_00023aac;
      iVar6 = 0;
      iVar4 = DAT_00023ab0;
      iVar7 = DAT_00023ab4;
      do {
        if (*(int **)(iVar4 + 0x1c) != param_1) {
          if (iVar4 != iVar7) {
            FUN_0001c7d0(iVar4,iVar7);
          }
          iVar6 = iVar6 + 1;
          iVar7 = iVar7 + 0x20;
        }
        iVar4 = iVar4 + 0x20;
        iVar5 = iVar5 + -1;
      } while (iVar5 != 0);
      *piVar2 = iVar6;
      FUN_000249f8(param_1[0xd]);
    }
    FUN_000249f8(param_1);
    FUN_00020af8(uVar1);
  }
  return;
}

