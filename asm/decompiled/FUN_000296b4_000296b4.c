// Function: FUN_000296b4
// Address: 000296b4


undefined8 FUN_000296b4(void)

{
  uint *puVar1;
  uint uVar2;
  ushort *puVar3;
  ushort *puVar4;
  undefined1 in_ZR;
  undefined8 uVar5;
  
  uVar5 = FUN_000297e8();
  puVar1 = DAT_00029c14;
  uVar2 = (uint)uVar5;
  if ((bool)in_ZR) {
    FUN_00020be4(5,uVar2);
    return uVar5;
  }
  puVar3 = (ushort *)(*DAT_00029c14 & 0xfffff7ff);
  if (uVar2 == 0xb) {
    (*(code *)*DAT_00029c18)();
    *DAT_00029c1c = 0;
  }
  else {
    puVar4 = puVar3;
    if (uVar2 == 0xd) {
      do {
        puVar3 = puVar4 + 1;
        *puVar4 = 0x20;
        puVar4 = puVar3;
      } while (((uint)puVar3 & 0x3f) != 0);
    }
    else if ((int)uVar2 < 0x20) {
      *DAT_00029c10 = (byte)((uVar2 & 0xf) << 4);
    }
    else {
      if (0x5f < uVar2) {
        uVar2 = uVar2 - 0x20;
      }
      *puVar3 = (ushort)uVar2 | (ushort)*DAT_00029c10 << 8;
      puVar3 = puVar3 + 1;
    }
    if (DAT_00029c20 <= puVar3) {
      puVar3 = (ushort *)0x6000000;
    }
    *puVar1 = (uint)puVar3;
  }
  return uVar5;
}

