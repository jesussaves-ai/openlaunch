// Function: FUN_06029c10
// Address:  06029c10
// Signature: undefined FUN_06029c10()


void FUN_06029c10(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 *unaff_r10;
  undefined4 *unaff_r11;
  
  uVar1 = *(undefined4 *)PTR_DAT_0602a2ec;
  software_interrupt(0x90000);
  uVar2 = unaff_r10[3];
  uVar3 = unaff_r10[2];
  *unaff_r11 = *unaff_r10;
  unaff_r11[1] = uVar1;
  unaff_r11[2] = uVar2;
  unaff_r11[3] = uVar3;
  return;
}

