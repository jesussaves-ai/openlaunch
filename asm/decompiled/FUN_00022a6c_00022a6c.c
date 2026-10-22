// Function: FUN_00022a6c
// Address: 00022a6c


uint FUN_00022a6c(uint *param_1,int param_2,int param_3)

{
  undefined4 uVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  
  uVar1 = FUN_00020ae4();
  if ((((param_1 == (uint *)0x0) || (param_3 != 0x10)) || ((*param_1 & 0x2000) == 0)) ||
     ((*param_1 & 1) != 1)) {
    uVar2 = FUN_0001cd28();
  }
  else {
    uVar4 = (uint)*(ushort *)(param_2 + 2);
    uVar2 = FUN_000236e4(param_1[1],*(undefined4 *)(param_2 + 4));
    if (uVar2 == 0) {
      while( true ) {
        puVar3 = (uint *)param_1[1];
        uVar2 = puVar3[1];
        if (uVar2 == 5 || uVar2 == 8) break;
        bVar5 = uVar2 == 0xc;
        bVar6 = uVar2 == 1;
        bVar7 = uVar2 == 2;
        bVar8 = uVar2 == 0;
        if (((bVar5 || bVar6) || bVar7) || bVar8) {
          uVar4 = puVar3[0xf];
        }
        if (((bVar5 || bVar6) || bVar7) || bVar8) {
          puVar3 = DAT_00023070;
        }
        if (((bVar5 || bVar6) || bVar7) || bVar8) {
          *puVar3 = uVar4;
        }
        if (((bVar5 || bVar6) || bVar7) || bVar8) {
          uVar2 = 0xffffffff;
        }
        if (((bVar5 || bVar6) || bVar7) || bVar8) goto LAB_00022ab8;
        if ((*param_1 & 0x4000) != 0) {
          uVar2 = FUN_0001cd40();
          goto LAB_00022ab8;
        }
        FUN_00020af8(uVar1);
        FUN_0001cad0();
        FUN_00020ae4();
      }
      uVar2 = 0;
    }
  }
LAB_00022ab8:
  FUN_00020af8(uVar1);
  return uVar2;
}

