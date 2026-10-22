// Function: FUN_060267a0
// Address:  060267a0
// Signature: undefined FUN_060267a0()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_060267a0(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  
  iVar1 = ram0x060273d0;
  if (((*(ushort *)(ram0x060273d0 + 4) & 0x100) == 0) &&
     (*(ushort *)(ram0x060273d0 + 4) = *(ushort *)(ram0x060273d0 + 4) | 0x100,
     (*(ushort *)(iVar1 + 4) & 0x100) != 0)) {
    software_interrupt(0x30000);
  }
  *(undefined2 *)(DAT_060273d8 + 0x20) = 1;
  FUN_06025518(0,DAT_060273dc,0x200);
  iVar3 = DAT_060273e8;
  iVar2 = DAT_060273e4;
  iVar1 = DAT_060273e0;
  *(ushort *)(DAT_060273e0 + 0x84) = *(ushort *)(DAT_060273e0 + 0x84) | 2;
  *(undefined2 *)(iVar1 + -0x7a) = 0x30;
  *(undefined2 *)(iVar3 + -0xca) = 0;
  software_interrupt(0x30000);
  *(undefined2 *)(iVar3 + 0x68) = 0;
  *(undefined2 *)(iVar3 + 0x60) = 0x100;
  uVar5 = 0;
  do {
    FUN_060268f4(*(undefined1 *)(iVar2 + uVar5 + 100),uVar5);
    uVar5 = uVar5 + 1;
  } while (uVar5 < 0x69);
  *(ushort *)(iVar3 + 0x84) = *(byte *)(iVar2 + 0x41) + 0x80 & 0xff7f;
  uVar4 = DAT_060273f0;
  if (*(char *)(iVar2 + 0x40) == '\x02') {
    uVar4 = FUN_060268d8(DAT_060273f0);
  }
  uVar5 = FUN_06026910(uVar4,0x1e);
  FUN_060268f4(uVar5 | 0x3f,0x1e);
  *(short *)(iVar3 + 0x68) = (short)DAT_060273f4;
  *(undefined2 *)(iVar3 + -0xca) = 1;
  *(ushort *)(iVar1 + 0x84) = *(ushort *)(iVar1 + 0x84) & 0xfffd;
  return CONCAT44(param_2,param_1);
}

