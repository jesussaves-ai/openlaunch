// Function: FUN_06026254
// Address:  06026254
// Signature: undefined FUN_06026254()


void FUN_06026254(void)

{
  undefined4 uVar1;
  undefined1 *puVar2;
  
  puVar2 = &DAT_05000000;
  do {
    *(undefined4 *)(puVar2 + 0x80) = DAT_06026738;
    *(undefined4 *)(puVar2 + 0xa0) = DAT_0602673c;
    *(undefined4 *)(puVar2 + 0xc0) = DAT_06026740;
    *(undefined4 *)(puVar2 + 0xe0) = DAT_06026744;
    *(undefined4 *)(puVar2 + 0x100) = DAT_06026748;
    *(undefined4 *)(puVar2 + 0x120) = DAT_0602674c;
    *(undefined4 *)(puVar2 + 0x140) = DAT_06026750;
    uVar1 = DAT_06026754;
    *(undefined4 *)(puVar2 + 0x84) = DAT_06026754;
    *(undefined4 *)(puVar2 + 0xa4) = uVar1;
    *(undefined4 *)(puVar2 + 0xc4) = uVar1;
    *(undefined4 *)(puVar2 + 0xe4) = uVar1;
    *(undefined4 *)(puVar2 + 0x104) = uVar1;
    *(undefined4 *)(puVar2 + 0x124) = uVar1;
    *(undefined4 *)(puVar2 + 0x144) = uVar1;
    puVar2 = (undefined1 *)((uint)puVar2 ^ 0x400);
  } while (((uint)puVar2 & 0x400) != 0);
  return;
}

