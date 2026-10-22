// Function: FUN_0001ddec
// Address: 0001ddec


void FUN_0001ddec(void)

{
  short sVar1;
  short sVar2;
  undefined2 uVar3;
  
  if (*DAT_0001e0d0 == 0) {
    return;
  }
  *(int *)(DAT_0001e0d4 + 0x30) = *(int *)(DAT_0001e0d4 + 0x30) + 1;
  sVar1 = *DAT_0001e0d8;
  sVar2 = *DAT_0001e0dc;
  if (sVar2 == 0) {
    if (sVar1 == 0) goto LAB_0001de50;
    *DAT_0001e0e4 = *DAT_0001e0e4 | 2;
    uVar3 = 1;
  }
  else if (sVar2 == 1) {
    if (sVar1 == 0) {
      uVar3 = 0;
    }
    else if (sVar1 == 2) {
      uVar3 = 2;
    }
    else {
      if ((*DAT_0001e0e8 & 1) != 0) {
        *DAT_0001e0e8 = *DAT_0001e0e8 & 0xfffe;
      }
      if ((*DAT_0001e0ec & 1) == 0) goto LAB_0001de50;
      FUN_000263c0();
      FUN_0001e15c();
      *DAT_0001e0f4 = *DAT_0001e0f0;
      *DAT_0001e0f8 = *DAT_0001e0f8 | 1;
      uVar3 = 3;
    }
  }
  else if (sVar2 == 2) {
    if (sVar1 == 2) {
      FUN_0001e00c();
      goto LAB_0001de50;
    }
    uVar3 = 1;
  }
  else if (sVar2 == 3) {
    if (*DAT_0001e0fc == 2) {
      uVar3 = 4;
    }
    else {
      if ((*DAT_0001e100 & 1) != 0) goto LAB_0001de50;
      uVar3 = 1;
    }
  }
  else if (sVar2 == 4) {
    if ((*DAT_0001e104 & 1) == 0) {
      uVar3 = 1;
    }
    else {
      if (*DAT_0001e108 == 2) goto LAB_0001de50;
      uVar3 = 3;
    }
  }
  else {
    if ((sVar2 != 6) || ((*DAT_0001e10c & 1) != 0)) goto LAB_0001de50;
    uVar3 = 1;
  }
  *DAT_0001e0e0 = uVar3;
LAB_0001de50:
  FUN_0001f21c();
  return;
}

