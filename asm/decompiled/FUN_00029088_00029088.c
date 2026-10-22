// Function: FUN_00029088
// Address: 00029088


void FUN_00029088(void)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 uVar3;
  
  puVar1 = DAT_00029b74;
  uVar3 = *DAT_00029b74;
  *DAT_00029b74 = DAT_00029b78;
  iVar2 = DAT_00029b7c;
  FUN_000298c8(*(undefined1 *)(DAT_00029b7c + 5));
  FUN_000296b4(0x3a);
  FUN_000298c8(*(undefined1 *)(iVar2 + 6));
  FUN_000296b4(0x3a);
  FUN_000298c8(*(undefined1 *)(iVar2 + 7));
  *puVar1 = uVar3;
  return;
}

