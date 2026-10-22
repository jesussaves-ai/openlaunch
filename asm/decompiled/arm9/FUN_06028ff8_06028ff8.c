// Function: FUN_06028ff8
// Address:  06028ff8
// Signature: undefined FUN_06028ff8()


undefined8 FUN_06028ff8(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  
  FUN_06029f00(0,DAT_06029798);
  uVar1 = DAT_060297a4;
  if (*DAT_0602979c == DAT_060297a0) {
    return CONCAT44(param_2,param_1);
  }
  uVar2 = FUN_060264a0((uint)*DAT_0602979c,DAT_060250c8);
  FUN_060264a0(uVar2,uVar1);
  FUN_06026f8c(0,0x12);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}

