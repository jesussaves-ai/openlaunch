// Function: FUN_06024bc0
// Address:  06024bc0
// Signature: undefined FUN_06024bc0()


void FUN_06024bc0(void)

{
  int *piVar1;
  int iVar3;
  int iVar4;
  undefined1 uVar5;
  int *piVar2;
  
  FUN_0602acc8();
  FUN_0602946c();
  uVar5 = *DAT_0602516c == 0x4000;
  if ((bool)uVar5) {
    FUN_06029f04(*DAT_06025164,DAT_06025168);
    FUN_0602b718(DAT_06025170,DAT_06025174,0x10);
    if ((bool)uVar5) {
      return;
    }
  }
  FUN_0602c104(DAT_06025178,DAT_0602517c);
  FUN_0602c1a8(DAT_06025180,DAT_06025184,0x10);
  piVar1 = DAT_06025188;
  iVar3 = DAT_0602518c;
  iVar4 = DAT_06025190;
  do {
    piVar2 = piVar1 + 1;
    FUN_0602a4e0(*piVar1,iVar3);
    iVar3 = iVar3 + 0x208;
    piVar1 = piVar1 + 2;
    FUN_0602c1a8(*piVar2,iVar4,0xc);
    iVar4 = iVar4 + 0x10;
  } while (*piVar1 != 0);
  return;
}

