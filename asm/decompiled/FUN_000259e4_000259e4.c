// Function: FUN_000259e4
// Address: 000259e4


uint FUN_000259e4(void)

{
  uint uVar1;
  bool bVar2;
  bool bVar3;
  bool bVar4;
  
  bVar2 = *DAT_00025cec == 0;
  uVar1 = 0;
  if (!bVar2) {
    uVar1 = *DAT_00025cf0;
  }
  bVar3 = (uVar1 & 1) == 0;
  if (!bVar2 && !bVar3) {
    uVar1 = *DAT_00025cf4;
  }
  bVar4 = (uVar1 & 4) != 0;
  if ((bVar2 || bVar3) || !bVar4) {
    uVar1 = 0;
  }
  if ((!bVar2 && !bVar3) && bVar4) {
    uVar1 = 1;
  }
  return uVar1;
}

