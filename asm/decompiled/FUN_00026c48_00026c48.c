// Function: FUN_00026c48
// Address: 00026c48


void FUN_00026c48(undefined4 param_1,int param_2)

{
  int iVar1;
  uint uVar2;
  
  iVar1 = DAT_00026e60;
  uVar2 = param_2 + 3U & 0xfffffffc;
  *(int *)(DAT_00026e60 + 0x24) = *(int *)(DAT_00026e60 + 0x24) + 1;
  *(uint *)(iVar1 + 0x28) = *(int *)(iVar1 + 0x28) + uVar2;
  *(uint *)(iVar1 + 0x2c) = *(int *)(iVar1 + 0x2c) + uVar2 + -0xc;
  FUN_00026c28(param_1,0,0);
  iVar1 = DAT_00026e64;
  *(short *)(DAT_00026e64 + 0x2c) = (short)DAT_00026e68;
  *(undefined2 *)(iVar1 + 0xa8) = 0x8000;
  *(undefined2 *)(iVar1 + 0xae) = 0xd;
  return;
}

