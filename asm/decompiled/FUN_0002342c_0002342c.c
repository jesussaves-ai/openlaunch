// Function: FUN_0002342c
// Address: 0002342c


int FUN_0002342c(void)

{
  int *piVar1;
  undefined4 uVar2;
  int *piVar3;
  
  uVar2 = FUN_00020ae4();
  piVar3 = (int *)FUN_000248c8(DAT_00023aa0);
  if (piVar3 != (int *)0x0) {
    piVar3[0x16] = 0;
    piVar3[0x17] = 0;
    piVar3[0x12] = 0;
    piVar3[0x13] = 0;
    piVar3[0x14] = 0;
    piVar3[0x15] = 0;
    piVar3[1] = 0;
    piVar3[0xe] = 0;
    piVar3[10] = 0;
    piVar3[9] = 0;
    piVar3[0xf] = 0;
    piVar3[0xd] = 0;
    piVar3[0x10] = 0;
    piVar3[0x11] = 0;
    piVar1 = DAT_00023aa4;
    *piVar3 = *DAT_00023aa4;
    *piVar1 = (int)piVar3;
  }
  FUN_00020af8(uVar2);
  return (int)piVar3;
}

