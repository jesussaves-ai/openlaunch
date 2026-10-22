// Function: FUN_000253fc
// Address: 000253fc


undefined4 FUN_000253fc(void)

{
  byte bVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  undefined1 auStack_6c [8];
  undefined1 auStack_64 [80];
  undefined4 local_14;
  undefined4 local_10;
  
  iVar4 = DAT_00025c70;
  iVar3 = *DAT_00025c28;
  if (iVar3 == 0) {
    iVar4 = FUN_00024d98(1,DAT_00025c2c,auStack_64);
    if (iVar4 != -1) {
      FUN_00024f7c(auStack_64);
    }
    return 1;
  }
  if (iVar3 == 1) {
    sVar2 = *DAT_00025c5c;
    if (sVar2 == 2) {
      if ((*DAT_00025c60 & 1) == 0) {
        uVar5 = 0;
      }
      else {
        uVar5 = 2;
      }
      return uVar5;
    }
    if (sVar2 == 3) {
      sVar2 = *DAT_00025c64;
      if (sVar2 != 2) {
        if (sVar2 != 1 && sVar2 != 3) {
          return 2;
        }
        return 3;
      }
    }
    else if (sVar2 != 4) {
      if (sVar2 != 6) {
        return 0;
      }
      return 6;
    }
    if (*DAT_00025c68 != '\0') {
      return 3;
    }
    iVar3 = *DAT_00025c6c;
    if (iVar3 == 0) goto LAB_00025680;
    if (*(int *)(iVar3 + 0x10) == 0) {
      if (*(int *)(DAT_00025c70 + 0x40) == 0) {
        FUN_00021754(iVar3,*(int *)(iVar3 + 0x1c) == 0);
        *DAT_00025c74 = 2;
        return 4;
      }
      *(int *)(iVar3 + 0x10) = *(int *)(DAT_00025c70 + 0x40);
      *(undefined4 *)(iVar3 + 0x18) = *(undefined4 *)(iVar4 + 0x44);
      *(undefined4 *)(iVar3 + 0x14) = *(undefined4 *)(iVar4 + 0x48);
      *(undefined4 *)(iVar3 + 0x1c) = *(undefined4 *)(iVar4 + 0x4c);
    }
  }
  else {
    if (iVar3 != 2) {
      if (iVar3 == 3) {
        return 5;
      }
      if (iVar3 == 4) {
        iVar4 = 0;
        iVar6 = 3;
        iVar3 = DAT_00025c30;
        do {
          if ((*(byte *)(iVar3 + 0x84) & 0x80) != 0) {
            iVar4 = iVar4 + 1;
            iVar3 = iVar3 + 0xac;
          }
          iVar6 = iVar6 + -1;
        } while (iVar6 != 0);
        iVar4 = FUN_00024d98(iVar4,DAT_00025c34,auStack_6c);
        if (iVar4 != -1) {
          FUN_00025db4(*(undefined4 *)(iVar4 + 0x70),*(undefined4 *)(iVar4 + 0x74),
                       *(undefined4 *)(iVar4 + 0x80),*(undefined4 *)(iVar4 + 0x78));
          bVar1 = *(byte *)(iVar4 + 0x85);
          *DAT_00025c38 = bVar1 & 3;
          *DAT_00025c3c = (char)((bVar1 & 0x70) >> 4);
          FUN_0001c7d0(iVar4 + 0x60,DAT_00025c40,0x10);
          *DAT_00025c44 = *(undefined1 *)(iVar4 + 0x87);
          *DAT_00025c48 = *(undefined1 *)(iVar4 + 0x88);
          *DAT_00025c4c = *(undefined1 *)(iVar4 + 0x89);
          FUN_0001c7d0(iVar4 + 0x8c,DAT_00025c50,0x20);
          *DAT_00025c54 = local_14;
          *DAT_00025c58 = local_10;
          FUN_00024f7c(auStack_6c);
        }
        return 1;
      }
      return 6;
    }
    iVar4 = FUN_000218ac();
    if (iVar4 == 1) {
      return 4;
    }
    if (iVar4 != 3) {
      FUN_00024ce0();
      *DAT_00025c84 = 0xffffffff;
      return 6;
    }
  }
  iVar4 = 0x1000000;
  do {
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  FUN_0002132c(*DAT_00025c78);
LAB_00025680:
  *DAT_00025c7c = 3;
  *DAT_00025c80 = *DAT_00025c80 | 0x10;
  return 5;
}

