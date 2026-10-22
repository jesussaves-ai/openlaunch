// Function: FUN_00024f7c
// Address: 00024f7c


void FUN_00024f7c(int param_1)

{
  char cVar1;
  undefined4 uVar2;
  undefined1 *puVar3;
  
  FUN_00024aa4(param_1 + 0x22,DAT_00025250);
  FUN_00024aa4(param_1 + 0x22,DAT_00025254);
  puVar3 = DAT_00025258 + 1;
  *DAT_00025258 = *(undefined1 *)(param_1 + 0x21);
  FUN_0001c7d0(param_1,puVar3);
  *DAT_0002525c = *(undefined1 *)(param_1 + 0x3c);
  uVar2 = FUN_0001c7d0(param_1 + 0x48,DAT_00025260,0x10);
  FUN_00029878(uVar2,DAT_00025264);
  if (*DAT_00025268 == '\0') {
    FUN_00029878(0,DAT_0002526c);
    FUN_00029840();
    uVar2 = FUN_000298e0(*DAT_00025270);
    FUN_00029878(uVar2,DAT_00025274);
    FUN_000298ac(*DAT_00025278);
  }
  else {
    FUN_00029878(*DAT_00025268,DAT_0002527c);
    FUN_00029840();
    FUN_000298f8(*DAT_00025280);
  }
  uVar2 = FUN_00029830();
  uVar2 = FUN_00029878(uVar2,DAT_00025284);
  FUN_0002995c(uVar2,DAT_00025288);
  uVar2 = FUN_00029830();
  uVar2 = FUN_00029878(uVar2,DAT_0002528c);
  FUN_0002995c(uVar2,DAT_00025290);
  uVar2 = FUN_00029830();
  uVar2 = FUN_00029878(uVar2,DAT_00025294);
  if (*DAT_00025298 != '\0') {
    FUN_00029890(uVar2,DAT_00025298 + 1);
  }
  uVar2 = FUN_00029830();
  FUN_00029878(uVar2,DAT_0002529c);
  FUN_00029914(*DAT_000252a0);
  uVar2 = FUN_00029830();
  FUN_00029878(uVar2,DAT_000252a4);
  *DAT_000252ac = *DAT_000252a8;
  FUN_00029914(*(undefined2 *)(param_1 + 0x34));
  uVar2 = FUN_00029830();
  uVar2 = FUN_00029878(uVar2,DAT_000252b0);
  if (*DAT_000252b4 == '\x01') {
    uVar2 = FUN_00029878(uVar2,DAT_000252b8);
  }
  if (*DAT_000252bc == '\x02') {
    FUN_00029878(uVar2,DAT_000252c0);
  }
  cVar1 = *DAT_000252c4;
  uVar2 = DAT_000252c8;
  if (cVar1 == '\x01') {
    uVar2 = DAT_000252cc;
  }
  if (cVar1 == '\x02') {
    uVar2 = DAT_000252d0;
  }
  if (cVar1 == '\x03') {
    uVar2 = DAT_000252d4;
  }
  if (cVar1 == '\x04') {
    uVar2 = DAT_000252d8;
  }
  FUN_00029878(cVar1,uVar2);
  if (*DAT_000252e0 != *DAT_000252dc) {
    FUN_000296b4(0x2f);
    cVar1 = *DAT_000252e4;
    uVar2 = DAT_000252e8;
    if (cVar1 == '\x01') {
      uVar2 = DAT_000252ec;
    }
    if (cVar1 == '\x02') {
      uVar2 = DAT_000252f0;
    }
    if (cVar1 == '\x03') {
      uVar2 = DAT_000252f4;
    }
    if (cVar1 == '\x04') {
      uVar2 = DAT_000252f8;
    }
    FUN_00029878(cVar1,uVar2);
  }
  FUN_00029840();
  FUN_000298c8(*DAT_000252fc);
  FUN_00029840();
  FUN_000298c8(*DAT_00025300);
  uVar2 = FUN_00029830();
  FUN_00029878(uVar2,DAT_00025304);
  FUN_000298e0(*DAT_00025308);
  FUN_00029840();
  FUN_000298e0(*DAT_0002530c);
  FUN_00029830();
  *DAT_00025310 = 1;
  *DAT_00025314 = *DAT_00025314 | 3;
  *DAT_00025318 = 1;
  return;
}

