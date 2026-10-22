// Function: FUN_000279fc
// Address: 000279fc


undefined4 FUN_000279fc(int param_1,uint param_2)

{
  uint *puVar1;
  int iVar2;
  undefined4 uVar3;
  
  puVar1 = DAT_00028258;
  uVar3 = DAT_00028254;
  *(short *)(param_1 + 10) = (short)param_2 + -8;
  if (*puVar1 == 0) {
    iVar2 = FUN_00026cac();
    if (iVar2 == 0) goto LAB_00027a44;
    FUN_00026c48(param_1,param_2);
LAB_00027a7c:
    uVar3 = 1;
  }
  else {
    iVar2 = FUN_00026cac();
    if (iVar2 != 0) {
      FUN_00026c48(uVar3,*puVar1);
      *puVar1 = 0;
LAB_00027a44:
      if (param_2 < 0x800) {
        FUN_0001c7d0(param_1,uVar3,param_2);
        *puVar1 = param_2;
        goto LAB_00027a7c;
      }
    }
    uVar3 = 0;
  }
  return uVar3;
}

