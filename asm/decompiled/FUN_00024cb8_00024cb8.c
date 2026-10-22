// Function: FUN_00024cb8
// Address: 00024cb8


int FUN_00024cb8(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = 0;
  iVar3 = 0x20;
  iVar2 = DAT_0002522c;
  do {
    if ((*(ushort *)(iVar2 + 0x36) & 0x8000) != 0) {
      iVar1 = iVar1 + 1;
    }
    iVar2 = iVar2 + 0x60;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return iVar1;
}

