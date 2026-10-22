// Function: FUN_000249f8
// Address: 000249f8


undefined4 FUN_000249f8(int param_1)

{
  uint *puVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar2 = FUN_00020ae4();
  uVar4 = param_1 - 0xc;
  if (*(short *)(param_1 + -8) != 0) {
    FUN_000296b4(0x4f);
    FUN_000296b4(0x4f);
    FUN_000296b4(0x50);
    FUN_000296b4(0x53);
  }
  *(undefined2 *)(param_1 + -8) = 2;
  puVar1 = DAT_00024a88;
  uVar3 = *DAT_00024a88;
  if (uVar3 == 0) {
    *DAT_00024a88 = uVar4;
  }
  if (uVar4 < uVar3) {
    *puVar1 = uVar4;
  }
  FUN_00020af8(uVar2);
  return 2;
}

