// Function: FUN_0001ff1c
// Address: 0001ff1c


void FUN_0001ff1c(int param_1)

{
  int iVar1;
  undefined1 auStack_20 [20];
  
  if ((*(byte *)(param_1 + 6) & 2) == 0) {
    *DAT_00020610 = 0;
  }
  FUN_0001c72c(param_1 + 0x51,0x10);
  iVar1 = FUN_00022078(*(undefined2 *)(param_1 + 2));
  FUN_000204f4(param_1,iVar1 + 4,DAT_00020614,0x10);
  FUN_0001c7d0(auStack_20,param_1 + 0x51,0x10);
  *DAT_00020618 = 1;
  return;
}

