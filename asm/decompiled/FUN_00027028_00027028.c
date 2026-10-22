// Function: FUN_00027028
// Address: 00027028


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00027028(void)

{
  ushort uVar1;
  int iVar2;
  ushort uVar3;
  
  iVar2 = DAT_000278d4;
  _DAT_04000214 = 0x1000000;
  uVar3 = 0;
  while( true ) {
    uVar1 = *(ushort *)(iVar2 + 0x10) & *(ushort *)(iVar2 + 0x12);
    uVar3 = uVar3 | uVar1;
    if (uVar1 == 0) break;
    *(ushort *)(iVar2 + 0x10) = uVar1;
  }
  if ((*DAT_000278d8 != 0) && ((*DAT_000278dc & 2) != 0)) {
    if ((uVar3 & 1) != 0) {
      FUN_00026e70();
    }
    if ((uVar3 & 2) != 0) {
      FUN_00026f4c();
    }
  }
  return;
}

