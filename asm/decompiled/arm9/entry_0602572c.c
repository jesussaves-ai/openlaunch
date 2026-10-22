// Function: entry
// Address:  0602572c
// Signature: undefined entry()


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void entry(void)

{
  *DAT_06025970 = 9;
  FUN_0602bfc8(0xffffffff);
  *DAT_06025974 = 0x6800;
  FUN_0602c374();
  FUN_06024480();
  FUN_0602c104(DAT_06025978,DAT_0602597c);
  FUN_0602c338();
  FUN_06025538();
  FUN_0602c454();
  FUN_06025c20();
  FUN_06025c58();
  FUN_060255e0();
  FUN_0602555c();
  FUN_06024494();
  FUN_06025590();
  FUN_06024494();
  FUN_0602bfb4(0x10000);
  _DAT_04000004 = 8;
  FUN_0602bfb4(1);
  FUN_06025b9c(0xd);
  FUN_0602b038();
  *DAT_06025980 = 0x22;
  FUN_0602af88();
  FUN_0602bfb4(0x40000);
  FUN_0602c338();
  FUN_06025b9c(1);
  FUN_06025b9c(2);
  FUN_0602c338();
  FUN_06025b9c(3);
  FUN_06025b9c(4);
  FUN_0602c338();
  FUN_0602bfc8(0xffffffff);
  _DAT_04000208 = 0;
  FUN_0602bf88();
  _DAT_040000ba = 0;
  _DAT_040000c6 = 0;
  _DAT_040000d2 = 0;
  _DAT_040000de = 0;
  FUN_0602c104(0x40000b0,0x40);
  FUN_0602c104(0x4000100,0x10);
  FUN_0602c1a8(DAT_06025984,DAT_06025988,0xc);
  *DAT_06025990 = *DAT_0602598c;
  if (*DAT_06025994 == 1) {
    *DAT_06025998 = 0x8000;
    FUN_0602bc8c();
  }
  FUN_06025b9c(7);
  FUN_06026e2c();
  FUN_06027158(*DAT_0602599c & 0xfefe);
  _DAT_04000298 = 1;
  DAT_04000300 = 1;
  _DAT_04000304 = DAT_060259a0;
  _DAT_04000204 = (undefined2)DAT_060259a4;
  if (*DAT_060259a8 != 0xff) {
    _DAT_04000000 = 0x10000;
    FUN_0602c104(&DAT_04000004,0x4a);
    _DAT_04001000 = 0x10000;
    FUN_0602c104(0x4001004,0x4a);
    _DAT_04000020 = 0x100;
    _DAT_04001020 = 0x100;
    _DAT_04000024 = 0x1000000;
    _DAT_04001024 = 0x1000000;
  }
  _DAT_04000208 = 0;
  _DAT_04000210 = 0;
  _DAT_04000214 = 0xffffffff;
  _DAT_04000184 = DAT_060259ac;
  FUN_0602c1a8(DAT_060259b0,DAT_060259b4,DAT_060259b8);
  FUN_0602c338();
                    /* WARNING: Could not recover jumptable at 0x06025958. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*DAT_060259bc)();
  return;
}

