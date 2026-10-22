// Function: FUN_0002013c
// Address: 0002013c


void FUN_0002013c(int param_1)

{
  byte bVar1;
  char *pcVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  
  uVar4 = (uint)CONCAT11(*(undefined1 *)(param_1 + 0x61),*(undefined1 *)(param_1 + 0x62));
  if (*DAT_00020634 == '\x03') {
    iVar6 = 0x20;
  }
  else {
    iVar6 = 0x10;
  }
  pcVar2 = (char *)(param_1 + 99);
  while( true ) {
    uVar5 = uVar4 - 2;
    if (uVar4 < 2) {
      return;
    }
    uVar3 = (uint)(byte)pcVar2[1];
    uVar4 = uVar5 - uVar3;
    if (uVar5 < uVar3) {
      return;
    }
    if (*pcVar2 == -0x23) break;
    pcVar2 = pcVar2 + 2 + uVar3;
  }
  if (iVar6 + 6U != uVar3) {
    return;
  }
  if (pcVar2[2] != '\0') {
    return;
  }
  if (pcVar2[3] != '\x0f') {
    return;
  }
  if (pcVar2[4] != -0x54) {
    return;
  }
  if (pcVar2[5] != '\x01') {
    return;
  }
  bVar1 = pcVar2[6];
  if (bVar1 == 0) {
    return;
  }
  if (3 < bVar1) {
    return;
  }
  *DAT_00020638 = bVar1;
  if (pcVar2[7] != '\0') {
    return;
  }
  FUN_0001c7d0(pcVar2 + 8,DAT_0002063c,iVar6);
  return;
}

