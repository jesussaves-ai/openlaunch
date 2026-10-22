// Function: FUN_0001d878
// Address: 0001d878


undefined8 FUN_0001d878(undefined4 param_1,uint param_2)

{
  undefined4 uVar1;
  undefined2 *puVar2;
  uint uVar3;
  
  puVar2 = DAT_0001dc9c;
  uVar1 = DAT_0001dc88;
  *(uint *)(DAT_0001dc9c + 2) = param_2 << 9 | param_2 & 0x70000000;
  *(uint *)(puVar2 + 0xe) = *(uint *)(puVar2 + 0xe) & 0x7c80fffa;
  puVar2[4] = puVar2[4] & 0xfffe;
  *puVar2 = (short)uVar1;
  do {
    if ((*(uint *)(puVar2 + 0xe) & 0x7f0000) != 0) {
      uVar3 = 0xffffffff;
      goto LAB_0001d924;
    }
  } while ((*(uint *)(puVar2 + 0xe) & 1) == 0);
  uVar3 = *(uint *)(puVar2 + 6) & 0xff;
LAB_0001d924:
  return CONCAT44(param_2,uVar3);
}

