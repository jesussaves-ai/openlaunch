// Function: FUN_06028844
// Address:  06028844
// Signature: undefined FUN_06028844()


undefined4 FUN_06028844(byte *param_1)

{
  undefined4 uVar1;
  byte bVar2;
  byte bVar3;
  
  bVar3 = param_1[1];
  if (bVar3 == 0) {
    bVar3 = param_1[2];
  }
  bVar2 = *param_1 | 0x20;
  bVar3 = bVar3 | 0x20;
  uVar1 = 0x41;
  if (((((bVar2 != 0x6e || bVar3 != 0x61) && (uVar1 = 0x43, bVar2 != 0x73 || bVar3 != 100)) &&
       (uVar1 = 0x55, bVar2 != 0x6b)) &&
      ((uVar1 = 0x56, bVar2 != 0x6e || bVar3 != 0x6f && (uVar1 = 0x57, bVar2 != 0x77)))) &&
     ((uVar1 = 0x58, bVar2 != 0x6d &&
      ((uVar1 = 0x59, bVar2 != 0x73 || bVar3 != 0x65 && (uVar1 = 0x5a, bVar2 != 99)))))) {
    uVar1 = 0;
  }
  return uVar1;
}

