// Function: FUN_0001f9c8
// Address: 0001f9c8


void FUN_0001f9c8(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  int extraout_r1;
  uint uVar3;
  uint *puVar4;
  int unaff_r4;
  uint unaff_r5;
  int unaff_r6;
  bool bVar5;
  uint local_64 [5];
  uint local_50 [5];
  uint local_3c [5];
  
  do {
    uVar2 = FUN_000204f4(param_1,param_2,param_3,param_4);
    FUN_0001c7d0(local_64,local_3c,uVar2);
    iVar1 = unaff_r6;
    while (iVar1 = iVar1 + -1, iVar1 != 0) {
      uVar2 = FUN_000204f4(local_64,uVar2,param_3,param_4);
      uVar3 = uVar2;
      puVar4 = local_64;
      do {
        *puVar4 = puVar4[5];
        puVar4[10] = puVar4[5] ^ puVar4[10];
        puVar4 = puVar4 + 1;
        uVar3 = uVar3 - 4;
      } while (uVar3 != 0);
    }
    *(char *)(param_1 + param_2 + -1) = *(char *)(param_1 + param_2 + -1) + '\x01';
    uVar3 = unaff_r5;
    if (uVar2 < unaff_r5) {
      uVar3 = uVar2;
    }
    FUN_0001c7d0(local_3c,unaff_r4,uVar3);
    uVar2 = extraout_r1 - unaff_r4;
    unaff_r4 = unaff_r4 + uVar2;
    bVar5 = uVar2 <= unaff_r5;
    unaff_r5 = unaff_r5 - uVar2;
  } while (bVar5 && unaff_r5 != 0);
  return;
}

