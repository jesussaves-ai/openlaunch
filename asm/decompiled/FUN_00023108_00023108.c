// Function: FUN_00023108
// Address: 00023108


void FUN_00023108(void)

{
  uint uVar1;
  int *piVar3;
  uint uVar4;
  bool bVar5;
  uint uVar2;
  
  uVar2 = *DAT_00023a80 + (uint)*DAT_00023a84;
  do {
    uVar1 = uVar2 + 1;
    if (0xffff < uVar1) {
      uVar1 = uVar2 - 0x3fff;
    }
    piVar3 = (int *)*DAT_00023a88;
    while( true ) {
      if (piVar3 == (int *)0x0) {
        *DAT_00023a80 = uVar1;
        return;
      }
      uVar4 = (uint)*(ushort *)(piVar3 + 0xc);
      bVar5 = uVar4 == uVar1;
      if (bVar5) {
        uVar4 = piVar3[1];
      }
      uVar2 = uVar1;
      if (bVar5 && (uVar4 & 0xc) == 0) break;
      piVar3 = (int *)*piVar3;
    }
  } while( true );
}

