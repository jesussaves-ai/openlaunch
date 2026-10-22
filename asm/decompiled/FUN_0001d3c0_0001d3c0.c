// Function: FUN_0001d3c0
// Address: 0001d3c0


void FUN_0001d3c0(void)

{
  int iVar1;
  undefined8 uVar2;
  
  FUN_0001d7f8(DAT_0001dba8,*DAT_0001dba4 & 0xfffffffe);
  FUN_0001d7f8(DAT_0001dbb0,*DAT_0001dbac);
  uVar2 = FUN_0001d628(DAT_0001dbb4);
  FUN_0001d760((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x6c,4);
  uVar2 = FUN_0001d628(DAT_0001dbb8);
  FUN_0001d760((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x74,4);
  FUN_0001d6d0();
  do {
    uVar2 = FUN_0001d628();
    iVar1 = FUN_0001d5a0((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x58);
  } while (iVar1 != 1);
  return;
}

