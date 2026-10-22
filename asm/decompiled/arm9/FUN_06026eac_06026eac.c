// Function: FUN_06026eac
// Address:  06026eac
// Signature: undefined FUN_06026eac()


void FUN_06026eac(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  if (*DAT_060274c0 == '\0') {
    return;
  }
  if ((*(byte *)(DAT_060274c4 + 0x12) & 2) == 0) {
    *DAT_060274c8 = (short)DAT_060274cc;
    *DAT_060274d0 = 0x7b;
    uVar2 = FUN_06026f8c(0,0x12);
    FUN_060254d0(uVar2,0xd);
    FUN_0602c104(DAT_060274d4,0x38);
    puVar1 = DAT_060274d8;
    *DAT_060274d8 = 0xffffffff;
    puVar1[2] = DAT_060274dc;
    return;
  }
  FUN_06026f8c(1,0x12);
  return;
}

