// Function: FUN_00022144
// Address: 00022144


undefined4 FUN_00022144(uint *param_1)

{
  byte bVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  byte *pbVar5;
  
  pbVar5 = (byte *)param_1[3];
  bVar1 = *pbVar5;
  uVar2 = FUN_00022078(*(undefined2 *)(pbVar5 + 2));
  if ((((uVar2 <= *param_1) && (FUN_00022444(param_1,uVar2), (*pbVar5 & 0xf0) == 0x40)) &&
      (iVar3 = FUN_00022504(param_1,0,(bVar1 & 0xf) * 4), DAT_0002235c == iVar3)) &&
     (uVar2 = FUN_00022078(*(undefined2 *)(pbVar5 + 6)), (uVar2 & 0xffff3fff) == 0)) {
    FUN_00022408(param_1,(bVar1 & 0xf) * -4);
    bVar1 = pbVar5[9];
    uVar4 = DAT_00022360;
    if (((bVar1 == 1) || (uVar4 = DAT_00022364, bVar1 == 6)) ||
       (uVar4 = DAT_00022368, bVar1 == 0x11)) {
      FUN_0001c968(param_1,*(undefined4 *)(pbVar5 + 0xc),*(undefined4 *)(pbVar5 + 0x10),uVar4);
      return 0;
    }
  }
  FUN_000223c8(param_1);
  return 0;
}

