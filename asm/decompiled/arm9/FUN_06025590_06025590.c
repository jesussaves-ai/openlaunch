// Function: FUN_06025590
// Address:  06025590
// Signature: undefined FUN_06025590()


void FUN_06025590(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  int iVar4;
  
  iVar1 = DAT_060256e8;
  *(undefined4 *)(DAT_060256e8 + 0x5c) = DAT_060256ec;
  *(undefined1 *)(iVar1 + 0x50) = 0x80;
  FUN_0602c338();
  uVar2 = DAT_060256f0;
  iVar4 = 0x8000;
  puVar3 = (undefined4 *)&DAT_03800000;
  do {
    *puVar3 = uVar2;
    iVar4 = iVar4 + -4;
    puVar3 = puVar3 + 1;
  } while (iVar4 != 0);
  FUN_0602c338();
  *(uint *)(iVar1 + 0x50) = *(uint *)(iVar1 + 0x50) & 0xff00ff00 | 0x99;
  return;
}

