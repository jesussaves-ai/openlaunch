// Function: FUN_000220a8
// Address: 000220a8


undefined4 FUN_000220a8(undefined4 *param_1,undefined4 param_2,undefined4 param_3)

{
  ushort uVar1;
  int iVar2;
  undefined4 uVar3;
  char *pcVar4;
  
  if (param_1 == (undefined4 *)0x0) {
    return 0;
  }
  pcVar4 = (char *)param_1[3];
  if (((*(short *)(pcVar4 + 2) == 0) ||
      (iVar2 = FUN_00022504(param_1,0,*param_1), iVar2 == DAT_00022358)) && (*pcVar4 == '\b')) {
    *pcVar4 = '\0';
    pcVar4[2] = '\0';
    pcVar4[3] = '\0';
    uVar1 = FUN_00022504(param_1,0,*param_1);
    *(ushort *)(pcVar4 + 2) = ~uVar1;
    uVar3 = FUN_0002223c(param_1,1,param_3,param_2);
    return uVar3;
  }
  FUN_000223c8(param_1);
  return 0;
}

