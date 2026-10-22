// Function: FUN_06026b48
// Address:  06026b48
// Signature: undefined FUN_06026b48()


undefined8 FUN_06026b48(undefined4 param_1,uint *param_2)

{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint *puVar5;
  int iVar6;
  
  uVar2 = param_2[1];
  iVar6 = 0x10;
  uVar4 = *param_2;
  puVar1 = DAT_0602743c;
  do {
    puVar5 = puVar1;
    uVar3 = uVar2 ^ *puVar5;
    uVar2 = (*(int *)(DAT_06027440 + ((uVar3 & 0xff000000) >> 0x16)) +
             *(int *)(DAT_06027440 + 0x400 + ((uVar3 & 0xff0000) >> 0xe)) ^
            *(uint *)(DAT_06027440 + 0x800 + ((uVar3 & 0xff00) >> 6))) +
            *(int *)(DAT_06027440 + 0xc00 + (uVar3 & 0xff) * 4) ^ uVar4;
    iVar6 = iVar6 + -1;
    uVar4 = uVar3;
    puVar1 = puVar5 + -1;
  } while (iVar6 != 0);
  uVar4 = puVar5[-2];
  *param_2 = puVar5[-1] ^ uVar2;
  param_2[1] = uVar4 ^ uVar3;
  return CONCAT44(param_2,param_1);
}

