// Function: FUN_00022804
// Address: 00022804


uint * FUN_00022804(uint param_1)

{
  undefined4 uVar1;
  uint *puVar2;
  int iVar3;
  
  uVar1 = FUN_00020ae4();
  iVar3 = 0x20;
  puVar2 = DAT_00023064;
  do {
    if ((*puVar2 & 0x8000) == 0) {
      *puVar2 = param_1 | 0xa000;
      puVar2[1] = 0;
      goto LAB_0002284c;
    }
    puVar2 = puVar2 + 2;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  puVar2 = (uint *)FUN_0001cd38();
LAB_0002284c:
  FUN_00020af8(uVar1);
  return puVar2;
}

