// Function: FUN_06027184
// Address:  06027184
// Signature: undefined FUN_06027184()


void FUN_06027184(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int iVar3;
  undefined4 uVar4;
  
  iVar3 = 0x14;
  puVar1 = DAT_06027534;
  puVar2 = DAT_06027538;
  do {
    uVar4 = *puVar1;
    *puVar1 = *puVar2;
    *puVar2 = uVar4;
    iVar3 = iVar3 + -4;
    puVar1 = puVar1 + 1;
    puVar2 = puVar2 + 1;
  } while (iVar3 != 0);
  return;
}

