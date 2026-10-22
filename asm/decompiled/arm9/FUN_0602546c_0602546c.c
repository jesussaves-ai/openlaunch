// Function: FUN_0602546c
// Address:  0602546c
// Signature: undefined FUN_0602546c()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 FUN_0602546c(int param_1,undefined4 param_2,int param_3)

{
  uint uVar1;
  undefined1 uVar2;
  uint uVar3;
  
  uVar3 = DAT_060256cc;
  if (param_3 != 1) {
    uVar3 = DAT_060256cc | 0x800;
  }
  _DAT_040001c0 = uVar3 | param_1 << 0x10;
  uVar1 = _DAT_040001c0;
  do {
  } while ((uVar3 & 0x80) != 0);
  if (param_3 == 1) {
    software_interrupt(0x30000);
  }
  DAT_040001c2 = (undefined1)(_DAT_040001c0 >> 0x10);
  DAT_04000208 = param_3 == 1;
  uVar2 = DAT_040001c2;
  _DAT_040001c0 = uVar1;
  return uVar2;
}

