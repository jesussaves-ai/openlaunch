// Function: FUN_000266b8
// Address: 000266b8


void FUN_000266b8(void)

{
  byte bVar1;
  undefined1 uVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  byte local_114 [64];
  undefined1 auStack_d4 [64];
  undefined1 uStack_94;
  char local_93;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  byte local_44;
  byte local_2e;
  byte local_2d;
  
  iVar6 = 6;
  iVar5 = DAT_00026dec;
  do {
    *(undefined1 *)(iVar5 + 0x84) = 0;
    iVar5 = iVar5 + 0xac;
    iVar6 = iVar6 + -1;
  } while (iVar6 != 0);
  iVar5 = (uint)*(ushort *)(DAT_00026df0 + 0x20) * 8;
  iVar6 = iVar5 + -0x400;
  FUN_0001ca30(iVar5 + -0x311,local_114,1);
  uVar8 = local_114[0] & 7;
  FUN_0001ca30(DAT_00026df4,local_114,1);
  bVar1 = local_114[0];
  if (local_114[0] == 0xff) {
    bVar1 = 0x10;
  }
  if (0x1f < bVar1) {
    FUN_0001ca30(iVar5 + -0x911,local_114,1);
    uVar8 = uVar8 | (local_114[0] & 7) << 3;
  }
  uVar7 = 0;
  iVar5 = DAT_00026df8;
  do {
    bVar1 = (byte)uVar8;
    uVar8 = uVar8 >> 1;
    if ((bool)(bVar1 & 1)) {
      FUN_0001ca30(iVar6,local_114,0x100);
      iVar3 = FUN_0001cf04(0,local_114,0x100);
      if ((((iVar3 == 0) && (local_2d != 0xff)) && (local_2d != 0xff)) &&
         ((local_2d < 2 || local_2d == 0x10) || local_2d == 0x13)) {
        *(undefined1 *)(iVar5 + 0x3c) = 0;
        *(undefined2 *)(iVar5 + 0x22) = 0;
        *(undefined2 *)(iVar5 + 0x24) = 0;
        *(undefined2 *)(iVar5 + 0x26) = 0;
        FUN_0001c7d0(auStack_d4,iVar5,0x20);
        *(undefined1 *)(iVar5 + 0x20) = 0;
        uVar2 = FUN_0001ca50();
        *(undefined1 *)(iVar5 + 0x21) = uVar2;
        *(byte *)(iVar5 + 0x85) = local_2e & 0xf;
        FUN_0001c7d0(&uStack_94,iVar5 + 0x60,0x10);
        *(undefined4 *)(iVar5 + 0x70) = local_54;
        *(undefined4 *)(iVar5 + 0x74) = local_50;
        *(undefined4 *)(iVar5 + 0x78) = local_4c;
        *(undefined4 *)(iVar5 + 0x7c) = local_48;
        uVar4 = FUN_00022088(-1 << (0x20 - local_44 & 0xff));
        *(undefined4 *)(iVar5 + 0x80) = uVar4;
        *(undefined1 *)(iVar5 + 0x86) = 0;
        if (2 < uVar7) {
          FUN_0001ca30(iVar6 + 0x100,local_114,0x100);
          iVar3 = FUN_0001cf04(0,local_114,0x100);
          if (iVar3 != 0) goto LAB_000268a8;
          *(char *)(iVar5 + 0x86) = local_93;
          if (local_93 != '\0') {
            if (*DAT_00026dfc == '\0') goto LAB_000268a8;
            FUN_0001c7d0(local_114,iVar5 + 0x8c,0x20);
          }
        }
        *(undefined1 *)(iVar5 + 0x84) = 0x80;
        iVar5 = iVar5 + 0xac;
      }
    }
LAB_000268a8:
    if (uVar7 < 2) {
      iVar6 = iVar6 + 0x100;
    }
    if (uVar7 == 2) {
      iVar6 = iVar6 + -0x800;
    }
    if (2 < uVar7) {
      iVar6 = iVar6 + 0x200;
    }
    uVar7 = uVar7 + 1;
    if (uVar7 == 6) {
      return;
    }
  } while( true );
}

