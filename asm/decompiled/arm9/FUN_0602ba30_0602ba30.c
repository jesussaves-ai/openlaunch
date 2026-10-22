// Function: FUN_0602ba30
// Address:  0602ba30
// Signature: undefined FUN_0602ba30()


undefined8 FUN_0602ba30(uint *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  bool bVar5;
  
  uVar1 = *param_1;
  uVar2 = param_1[2];
  uVar3 = param_1[3];
  if (uVar3 != 0) {
    do {
      uVar4 = 0x1000;
      if (uVar3 < 0x1000) {
        uVar4 = 0x200;
      }
      if ((uVar1 & uVar4 - 1) != 0) {
        uVar4 = 0x200;
      }
      FUN_0602bd94(uVar1 >> 8 | 0xb7000000,uVar1 << 0x18,uVar4,uVar2,param_1,param_2,param_3,param_4
                  );
      uVar1 = uVar1 + uVar4;
      uVar2 = uVar2 + uVar4;
      bVar5 = uVar4 <= uVar3;
      uVar3 = uVar3 - uVar4;
    } while (bVar5 && uVar3 != 0);
  }
  return CONCAT44(param_2,param_1);
}

