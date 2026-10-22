// Function: FUN_060287b8
// Address:  060287b8
// Signature: undefined FUN_060287b8()


void FUN_060287b8(void)

{
  undefined2 *puVar1;
  undefined4 uVar2;
  undefined2 extraout_r1;
  
  uVar2 = FUN_06028504();
  puVar1 = DAT_0602966c;
  *(undefined4 *)(DAT_0602966c + 2) = uVar2;
  *(undefined4 *)(puVar1 + 0xe) = 0;
  FUN_06028504();
  puVar1[4] = puVar1[4] & 0xfffe;
  FUN_06028504();
  *puVar1 = extraout_r1;
  do {
    FUN_06028504();
    if ((*(uint *)(puVar1 + 0xe) & 0x7f0000) != 0) goto LAB_06028830;
  } while ((*(uint *)(puVar1 + 0xe) & 1) == 0);
  FUN_06028504();
  if ((*(uint *)(puVar1 + 0xe) & 0x400000) == 0) {
    uVar2 = 0;
  }
  else {
LAB_06028830:
    uVar2 = 0xffffffff;
  }
  FUN_06028504(uVar2);
  return;
}

