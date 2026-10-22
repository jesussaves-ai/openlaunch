// Function: FUN_000243bc
// Address: 000243bc


void FUN_000243bc(void)

{
  ushort *puVar1;
  uint uVar2;
  int *piVar4;
  uint uVar3;
  
  uVar3 = *DAT_0002486c + (uint)*DAT_00024870;
  do {
    uVar2 = uVar3 + 1;
    if (0xffff < uVar2) {
      uVar2 = uVar3 - 0x3fff;
    }
    piVar4 = (int *)*DAT_00024874;
    do {
      if (piVar4 == (int *)0x0) {
        *DAT_0002486c = uVar2;
        return;
      }
      puVar1 = (ushort *)(piVar4 + 4);
      piVar4 = (int *)*piVar4;
      uVar3 = uVar2;
    } while (*puVar1 != uVar2);
  } while( true );
}

