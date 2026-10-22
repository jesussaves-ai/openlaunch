// Function: FUN_0001d7c8
// Address: 0001d7c8


void FUN_0001d7c8(undefined4 param_1)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  
  FUN_0001d6b8();
  uVar2 = DAT_0001dc78;
  puVar1 = DAT_0001dc74;
  *DAT_0001dc74 = 6;
  puVar1[1] = param_1;
  FUN_0001d958(puVar1,uVar2,8);
  FUN_0001d658();
  return;
}

