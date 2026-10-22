// Function: FUN_06029a50
// Address:  06029a50
// Signature: undefined FUN_06029a50()


undefined8 FUN_06029a50(undefined4 param_1,undefined4 param_2)

{
  undefined1 uVar1;
  undefined1 uVar2;
  
  FUN_0602946c(param_1);
  FUN_06029b5c();
  FUN_0602c1a8(DAT_0602a288,DAT_0602a28c,0x1000);
  FUN_0602c1a8(DAT_0602a290,DAT_0602a294,0x160);
  uVar2 = (*(byte *)(DAT_0602a298 + 0x12) & 2) == 0;
  if ((bool)uVar2) {
    *(undefined1 *)(DAT_0602a298 + 0x1af) = 3;
  }
  FUN_06029960();
  uVar1 = *DAT_0602a29c;
  FUN_06024540();
  FUN_060288d4(uVar1);
  FUN_0602946c(param_1);
  FUN_06029b5c();
  FUN_06029c10();
  FUN_06029c10();
  FUN_06029c10();
  FUN_06029c10();
  while( true ) {
    FUN_06029b80();
    FUN_06029b80();
    FUN_06029b80();
    FUN_06029b80();
    *DAT_0602a2d4 = 0;
    FUN_06029590();
    if ((bool)uVar2) break;
    FUN_06029b74();
    if (!(bool)uVar2) {
      FUN_06029b74();
    }
    if (!(bool)uVar2) {
      FUN_06029b74();
    }
    if (!(bool)uVar2) {
      FUN_06029b74();
    }
    if ((bool)uVar2) {
      FUN_06029b5c();
    }
  }
  return CONCAT44(param_2,param_1);
}

