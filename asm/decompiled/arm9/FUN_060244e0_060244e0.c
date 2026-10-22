// Function: FUN_060244e0
// Address:  060244e0
// Signature: undefined FUN_060244e0()


void FUN_060244e0(void)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  piVar2 = DAT_060250e8;
  puVar1 = DAT_060250e4;
  iVar6 = *DAT_060250e8;
  uVar5 = *DAT_060250e4;
LAB_060244f4:
  do {
    FUN_0602544c();
    uVar3 = FUN_0602441c();
    uVar4 = uVar3 & ~(*DAT_060250ec & uVar3);
    *DAT_060250ec = *DAT_060250ec & uVar3;
    if (uVar4 == uVar5) {
      iVar6 = iVar6 + -1;
      if (iVar6 != 0) goto LAB_060244f4;
      iVar6 = 3;
    }
    else {
      iVar6 = 0xf;
    }
    uVar5 = uVar4;
    if (uVar4 != 0) {
      *puVar1 = uVar4;
      *piVar2 = iVar6;
      return;
    }
  } while( true );
}

