// Function: FUN_06026168
// Address:  06026168
// Signature: undefined FUN_06026168()


void FUN_06026168(void)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)&DAT_04000000;
  do {
    *puVar1 = 0x10000;
    *(short *)(puVar1 + 2) = (short)DAT_0602672c;
    puVar1[8] = 0x100;
    puVar1[9] = 0x1000000;
    puVar1[10] = 0;
    puVar1[0xb] = 0;
    puVar1[0x1b] = 0;
    puVar1 = (undefined4 *)((uint)puVar1 ^ 0x1000);
  } while (((uint)puVar1 & 0x1000) != 0);
  return;
}

