// Function: FUN_0602a538
// Address:  0602a538
// Signature: undefined FUN_0602a538()


void FUN_0602a538(byte *param_1,ushort *param_2)

{
  ushort uVar1;
  
  do {
    uVar1 = (ushort)*param_1;
    if ((uVar1 != 0x3f) && ((*param_2 ^ uVar1) != 0)) {
      if ((*param_2 ^ uVar1) != 0x20) {
        return;
      }
      if ((uVar1 & 0xffdf) < 0x41) {
        return;
      }
      if (0x5a < (uVar1 & 0xffdf)) {
        return;
      }
    }
    param_1 = param_1 + 1;
    param_2 = param_2 + 1;
    if (uVar1 == 0) {
      return;
    }
  } while( true );
}

