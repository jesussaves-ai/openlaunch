// Function: FUN_000226fc
// Address: 000226fc


void FUN_000226fc(void)

{
  undefined4 uVar1;
  uint uVar2;
  uint *puVar3;
  int iVar4;
  
  uVar1 = FUN_00020ae4();
  iVar4 = 0x20;
  puVar3 = DAT_00023058;
  do {
    uVar2 = *puVar3;
    if ((uVar2 & 0x1000) != 0) {
      if ((*(int *)(puVar3[1] + 4) == 0xc) || ((uVar2 & DAT_0002305c) == 0)) {
        FUN_000228a0(puVar3);
      }
      else {
        *puVar3 = uVar2 - 0x10000;
      }
    }
    puVar3 = puVar3 + 2;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  FUN_00020af8(uVar1);
  return;
}

