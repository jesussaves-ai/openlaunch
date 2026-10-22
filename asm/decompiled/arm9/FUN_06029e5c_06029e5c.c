// Function: FUN_06029e5c
// Address:  06029e5c
// Signature: undefined FUN_06029e5c()


undefined8 FUN_06029e5c(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int extraout_r1;
  
  iVar1 = DAT_0602a384;
  *(ushort *)(DAT_0602a384 + 0xe0) = *(ushort *)(DAT_0602a384 + 0xe0) & 0xfffc;
  *(ushort *)(iVar1 + 0xe0) = *(ushort *)(iVar1 + 0xe0) | 3;
  *(ushort *)(iVar1 + 8) = *(ushort *)(iVar1 + 8) & 0xfffe | 0x100;
  FUN_06028838();
  *(ushort *)(iVar1 + 2) = *(ushort *)(extraout_r1 + 0x42) | 0x400;
  *(undefined2 *)(iVar1 + 0x24) = 0;
  *(undefined2 *)(iVar1 + 0x28) = 0xd0;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0x7fff;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) | 0x100;
  *(ushort *)(iVar1 + 0x28) = *(ushort *)(iVar1 + 0x28) & 0xfeff;
  *(undefined2 *)(iVar1 + 0x24) = 0x120;
  *(undefined2 *)(iVar1 + 0xd8) = 2;
  *(undefined4 *)(iVar1 + 0x100) = DAT_0602a388;
  return CONCAT44(param_2,param_1);
}

