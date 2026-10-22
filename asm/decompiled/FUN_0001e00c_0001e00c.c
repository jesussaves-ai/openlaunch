// Function: FUN_0001e00c
// Address: 0001e00c


void FUN_0001e00c(void)

{
  int iVar1;
  undefined2 uVar2;
  
  if (*DAT_0001e12c == '\0') {
    FUN_0001f1cc(DAT_0001e130);
    FUN_0001f1cc(DAT_0001e134);
    do {
      FUN_00026318();
    } while ((*DAT_0001e13c & 1 << (*DAT_0001e138 & 0xff)) == 0);
    *DAT_0001e140 = *DAT_0001e138;
    uVar2 = FUN_0001dfc8();
    *(undefined2 *)(DAT_0001e144 + 0xc) = uVar2;
    FUN_0001f1cc();
    iVar1 = DAT_0001e148;
    uVar2 = (undefined2)(*DAT_0001e14c << 6);
    *(undefined2 *)(DAT_0001e148 + 0xe) = uVar2;
    *(undefined2 *)(iVar1 + 0x10) = uVar2;
    *(undefined2 *)(iVar1 + 0x14) = uVar2;
    *(undefined1 *)(iVar1 + 0x13) = 1;
    FUN_0001f1cc();
    FUN_0001f1cc(DAT_0001e150);
    *DAT_0001e154 = 1;
    FUN_0001f1cc(DAT_0001e158);
  }
  return;
}

