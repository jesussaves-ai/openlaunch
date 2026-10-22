// Function: FUN_06027f50
// Address:  06027f50
// Signature: undefined FUN_06027f50()


undefined8 FUN_06027f50(uint param_1,uint param_2)

{
  undefined4 uVar1;
  undefined2 *puVar2;
  uint uVar3;
  
  puVar2 = DAT_060283c4;
  uVar1 = DAT_060283b4;
  *(uint *)(DAT_060283c4 + 2) = param_1 | param_2 << 9 | param_2 & 0x70000000 | 0x80000000;
  *(uint *)(puVar2 + 0xe) = *(uint *)(puVar2 + 0xe) & 0x7c80fffa;
  puVar2[4] = puVar2[4] & 0xfffe;
  *puVar2 = (short)uVar1;
  do {
    if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
      uVar3 = 0xffffffff;
      goto LAB_06027fe4;
    }
  } while ((*(uint *)(puVar2 + 0xe) & 1) == 0);
  uVar3 = *(uint *)(puVar2 + 6) & 0xff;
LAB_06027fe4:
  return CONCAT44(param_2,uVar3);
}

