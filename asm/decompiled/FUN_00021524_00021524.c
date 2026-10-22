// Function: FUN_00021524
// Address: 00021524


void FUN_00021524(void)

{
  char cVar1;
  char *pcVar2;
  char *pcVar3;
  
  *DAT_00021c34 = 0;
  *DAT_00021c38 = 0;
  *DAT_00021c3c = 0;
  *DAT_00021c40 = 0;
  pcVar2 = DAT_00021c44;
  pcVar3 = DAT_00021c48;
  do {
    cVar1 = *pcVar2;
    *pcVar3 = cVar1;
    pcVar2 = pcVar2 + 1;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  return;
}

