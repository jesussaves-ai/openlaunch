// Function: FUN_0001d82c
// Address: 0001d82c


int FUN_0001d82c(void)

{
  int iVar1;
  undefined8 uVar2;
  
  FUN_0001d6b8();
  FUN_0001d69c(8);
  iVar1 = FUN_0001d658();
  if (iVar1 == -1) {
    uVar2 = FUN_0001d658();
    FUN_0001d750((int)uVar2,(int)((ulonglong)uVar2 >> 0x20),(int)uVar2 + -4);
    iVar1 = *DAT_0001dc84;
  }
  return iVar1;
}

