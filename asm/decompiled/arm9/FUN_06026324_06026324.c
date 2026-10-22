// Function: FUN_06026324
// Address:  06026324
// Signature: undefined FUN_06026324()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_06026324(void)

{
  uint *puVar1;
  uint uVar2;
  ushort *puVar3;
  ushort *puVar4;
  undefined1 in_ZR;
  undefined8 uVar5;
  
  uVar5 = FUN_06026448();
  puVar1 = DAT_06026774;
  uVar2 = (uint)uVar5;
  if ((bool)in_ZR) {
    FUN_0602bff0(5,uVar2);
    return uVar5;
  }
  puVar3 = (ushort *)(*DAT_06026774 & 0xfffff7ff);
  if (uVar2 == 10) {
    puVar4 = (ushort *)(DAT_0602677c + 0x180);
  }
  else {
    if (uVar2 == 0xb) {
      (*(code *)*DAT_06026784)();
      *DAT_06026788 = 0;
      return uVar5;
    }
    if (uVar2 == 0xc) {
      FUN_06026304();
      return uVar5;
    }
    if (uVar2 == 0xd) {
      do {
        puVar4 = puVar3 + 1;
        *puVar3 = 0x20;
        puVar3 = puVar4;
      } while (((uint)puVar4 & 0x3f) != 0);
    }
    else if (uVar2 == 0xe) {
      puVar4 = (ushort *)(DAT_06026778 + 0xc0);
    }
    else {
      if (uVar2 == 0xf) {
        FUN_06026244();
        _DAT_04000000 = DAT_06026780;
        _DAT_04001000 = DAT_06026780;
        return uVar5;
      }
      if ((int)uVar2 < 0x20) {
        *DAT_06026770 = ((uVar2 & 0xf) + 4) * 0x10;
        return uVar5;
      }
      if (0x7f < uVar2) {
        uVar2 = 0x7f;
      }
      puVar4 = puVar3 + 1;
      *puVar3 = (ushort)uVar2 | (ushort)(*DAT_06026770 << 8);
    }
  }
  *puVar1 = (uint)puVar4;
  return uVar5;
}

