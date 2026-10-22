// Function: FUN_00025a14
// Address: 00025a14


void FUN_00025a14(void)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  undefined1 *puVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined1 auStack_4c [16];
  undefined1 auStack_3c [6];
  undefined1 auStack_36 [6];
  undefined1 auStack_30 [8];
  int local_28;
  short local_24;
  undefined2 local_22;
  
  if (*DAT_00025cf8 == 0) {
    *DAT_00025cf8 = 1;
    if (*DAT_00025cfc != 0) {
      if (((*DAT_00025d00 & 4) == 0) && ((*DAT_00025d04 & 1) != 0)) {
        *DAT_00025d00 = *DAT_00025d00 | 4;
        uVar1 = FUN_00021dd8(DAT_00025d08,DAT_00025d0c);
        *DAT_00025d10 = uVar1;
        *DAT_00025d18 = *DAT_00025d14;
      }
      if (*DAT_00025d1c == 2) {
        uVar2 = *DAT_00025d20 | 8;
      }
      else {
        uVar2 = *DAT_00025d20 & 0xfffffff7;
      }
      *DAT_00025d20 = uVar2;
      iVar6 = 0x50;
      do {
        if (*DAT_00025d24 == *DAT_00025d28) break;
        iVar7 = *DAT_00025d2c;
        uVar2 = FUN_00024bb0(iVar7,4);
        if ((0x1f < uVar2) && (uVar3 = FUN_00024bb0(iVar7,6), (DAT_00025d30 & uVar3) == 8)) {
          FUN_00024b60(iVar7,0x2c,auStack_4c);
          iVar4 = FUN_00024ac0(DAT_00025d34,auStack_3c);
          if (iVar4 == 0) {
            iVar4 = FUN_00024ac0(DAT_00025d38,auStack_3c);
          }
          if (((iVar4 != 0) && (local_28 == DAT_00025d3c && local_24 == 0)) &&
             (iVar4 = FUN_00022384(0xe,uVar2 - 0x20), iVar4 != 0)) {
            iVar8 = *(int *)(iVar4 + 0xc);
            FUN_00024aa4(auStack_3c,iVar8);
            if ((uVar3 & 0x200) == 0) {
              puVar5 = auStack_36;
            }
            else {
              puVar5 = auStack_30;
            }
            FUN_00024aa4(puVar5,iVar8 + 6);
            *(undefined2 *)(iVar8 + 0xc) = local_22;
            uVar3 = iVar7 + 0x16;
            if (0x17ff < uVar3) {
              uVar3 = iVar7 - 0x17ea;
            }
            FUN_00024b60(uVar3,uVar2 - 0x20,iVar8 + 0xe);
            FUN_00024a8c(0x52,iVar8);
            FUN_00021eac(*DAT_00025d40,iVar4);
          }
        }
        if (*DAT_00025d44 != 0) {
          uVar3 = iVar7 + 6;
          if (0x17ff < uVar3) {
            uVar3 = iVar7 - 0x17fa;
          }
          FUN_0001c968(uVar3,uVar2);
        }
        uVar2 = iVar7 + ((uVar2 + 0xf & 0xfffffffc) >> 1);
        if (0x17ff < uVar2) {
          uVar2 = uVar2 - 0x1800;
        }
        *DAT_00025d48 = uVar2;
        iVar6 = iVar6 + -1;
      } while (iVar6 != 0);
    }
    *DAT_00025d4c = 0;
  }
  return;
}

