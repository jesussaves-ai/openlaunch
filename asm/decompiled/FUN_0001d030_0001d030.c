// Function: FUN_0001d030
// Address: 0001d030


void FUN_0001d030(void)

{
  int iVar1;
  int *piVar2;
  int *piVar3;
  undefined1 auStack_8c [100];
  undefined1 auStack_28 [20];
  
  *DAT_0001db38 = *DAT_0001db38 + 1;
  FUN_0001fc7c(auStack_8c);
  piVar2 = DAT_0001db3c;
  do {
    piVar3 = piVar2 + 1;
    iVar1 = *piVar2;
    piVar2 = piVar2 + 2;
    FUN_0001fcd4(auStack_8c,iVar1,*piVar3);
  } while (*piVar2 != 0);
  thunk_FUN_0001fe08(auStack_28,auStack_8c);
  FUN_0001f9c8(auStack_28,0x14,DAT_0001db40,3);
  return;
}

