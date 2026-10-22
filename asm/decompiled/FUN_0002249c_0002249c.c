// Function: FUN_0002249c
// Address: 0002249c


uint FUN_0002249c(uint *param_1,uint param_2,uint param_3,int param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  
  iVar1 = FUN_00022078(*param_1);
  if ((*param_1 & 1) != 0) {
    *(undefined1 *)(param_1[3] + *param_1) = 0;
  }
  iVar2 = FUN_00022504(param_1,0,*param_1);
  uVar3 = iVar2 + param_4 + (param_2 >> 0x10) + (param_3 >> 0x10) + (param_2 & 0xffff) +
                  (param_3 & 0xffff) + iVar1;
  bVar5 = CARRY4(uVar3,uVar3 * 0x10000);
  uVar3 = uVar3 * 0x10001;
  uVar4 = uVar3;
  if (bVar5) {
    uVar4 = uVar3 + 0x10000;
  }
  if (bVar5 && 0xfffeffff < uVar3) {
    uVar4 = uVar4 + 0x10000;
  }
  return uVar4 >> 0x10;
}

