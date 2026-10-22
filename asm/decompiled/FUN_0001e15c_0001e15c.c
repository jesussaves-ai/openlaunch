// Function: FUN_0001e15c
// Address: 0001e15c


void FUN_0001e15c(void)

{
  byte bVar1;
  char cVar2;
  char cVar3;
  int iVar4;
  undefined2 uVar5;
  undefined1 uVar6;
  undefined1 uVar7;
  undefined1 uVar8;
  uint uVar9;
  
  FUN_0001f1cc(DAT_0001e320);
  iVar4 = DAT_0001e324;
  *(undefined2 *)(DAT_0001e324 + 0xe) = 200;
  *(undefined2 *)(iVar4 + 0x10) = 200;
  *(undefined2 *)(iVar4 + 0x14) = 200;
  *(undefined1 *)(iVar4 + 0x13) = 5;
  FUN_0001f1cc();
  bVar1 = *DAT_0001e328;
  *DAT_0001e32c = (ushort)bVar1;
  *DAT_0001e330 = (ushort)bVar1;
  uVar5 = FUN_0001dfc8();
  *(undefined2 *)(DAT_0001e334 + 0xc) = uVar5;
  FUN_0001f1cc();
  if (*DAT_0001e338 == '\x02') {
    uVar9 = 0;
    do {
      FUN_0001c72c(DAT_0001e33c,8);
      FUN_0001c7d0(DAT_0001e340,DAT_0001e344,0x20);
      iVar4 = DAT_0001e348;
      *(char *)(DAT_0001e348 + 8) = (char)uVar9;
      *(undefined1 *)(iVar4 + 9) = 2;
      uVar6 = 3;
      if (uVar9 != 0) {
        uVar6 = 1;
      }
      *(undefined1 *)(iVar4 + 10) = uVar6;
      uVar6 = 0xd;
      if (*DAT_0001e34c == '\x01') {
        uVar6 = 5;
      }
      if (*DAT_0001e34c == '\x03') {
        uVar6 = 0x10;
      }
      *(undefined1 *)(iVar4 + 0xb) = uVar6;
      FUN_0001f1cc();
      uVar9 = uVar9 + 1;
    } while (uVar9 < 4);
  }
  FUN_0001f1cc(DAT_0001e350);
  FUN_0001f1cc(DAT_0001e354);
  FUN_0001f1cc(DAT_0001e358);
  FUN_0001f1cc(DAT_0001e35c);
  FUN_0001f1cc(DAT_0001e360);
  FUN_0001f1cc(DAT_0001e364);
  FUN_0001f1cc(DAT_0001e368);
  FUN_0001f1cc(DAT_0001e36c);
  FUN_0001f1cc(DAT_0001e370);
  *DAT_0001e374 = 1;
  cVar2 = *DAT_0001e378;
  if (cVar2 != '\0') {
    *DAT_0001e37c = 1;
  }
  iVar4 = DAT_0001e388;
  uVar6 = *DAT_0001e380;
  cVar3 = *DAT_0001e384;
  uVar7 = 1;
  if (cVar3 == '\x02') {
    uVar7 = 2;
  }
  uVar8 = 1;
  if (cVar2 == '\x01') {
    uVar8 = 3;
  }
  if (cVar2 == '\x02') {
    uVar8 = 5;
  }
  *(undefined1 *)(DAT_0001e388 + 1) = uVar7;
  *(undefined1 *)(iVar4 + 2) = uVar8;
  *(undefined1 *)(iVar4 + 3) = uVar6;
  *(char *)(iVar4 + 5) = cVar3;
  FUN_0001c7d0(DAT_0001e38c,DAT_0001e390,0x21);
  uVar5 = FUN_0001dfc8(*DAT_0001e394);
  *DAT_0001e398 = uVar5;
  FUN_00025f60(DAT_0001e39c,DAT_0001e3a0);
  FUN_0001f1cc(DAT_0001e3a4);
  return;
}

