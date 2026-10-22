// Function: FUN_0002462c
// Address: 0002462c


int FUN_0002462c(void)

{
  int *piVar1;
  int iVar2;
  
  piVar1 = (int *)FUN_000248c8(0x1c);
  if (piVar1 != (int *)0x0) {
    FUN_00020ae4();
    piVar1[3] = 0;
    *(undefined2 *)((int)piVar1 + 0x12) = 0;
    piVar1[2] = 0;
    *(undefined2 *)(piVar1 + 4) = 0;
    piVar1[5] = 0;
    piVar1[6] = 0;
    piVar1[1] = 0;
    iVar2 = *DAT_00024880;
    *DAT_00024880 = (int)piVar1;
    *piVar1 = iVar2;
    FUN_00020af8();
  }
  return (int)piVar1;
}

