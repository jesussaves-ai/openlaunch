// Function: FUN_0001d42c
// Address: 0001d42c


void FUN_0001d42c(void)

{
  undefined4 uVar1;
  undefined8 uVar2;
  
  uVar2 = FUN_0001d628();
  uVar1 = FUN_0001d5a0((int)uVar2,(int)((ulonglong)uVar2 >> 0x20) + 0x54);
  *DAT_0001dbbc = uVar1;
  uVar1 = FUN_0001d5a0(uVar1,*DAT_0001dbc0);
  FUN_0001d5a0(uVar1,*DAT_0001dbc4 + 0x10);
  return;
}

