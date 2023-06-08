#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t* dest, uint8_t subcode) {
  bool invert = subcode & 0x1;
  enum {
    CC_O, CC_NO, CC_B,  CC_NB,
    CC_E, CC_NE, CC_BE, CC_NBE,
    CC_S, CC_NS, CC_P,  CC_NP,
    CC_L, CC_NL, CC_LE, CC_NLE
  };

  // TODO: Query EFLAGS to determine whether the condition code is satisfied.
  // dest <- ( cc is satisfied ? 1 : 0)
  switch (subcode & 0xe) {
    case CC_O:
	    rtl_get_OF(dest); //O
	    break;	
    case CC_B:
	    rtl_get_CF(dest); //2
	    break;
    case CC_E:
	    rtl_get_ZF(dest); //4
	    break;
    case CC_BE:
	    assert(dest!=&t0);
	    rtl_get_CF(dest); //6
	    rtl_get_ZF(&t0);
     	    rtl_or(dest,dest,&t0);
     	    break;
    case CC_S:
	    rtl_get_SF(dest); //8
	    break;
    case CC_L:
	    assert(dest!=&t0);
	    rtl_get_SF(dest); //C
	    rtl_get_OF(&t0);
      	    rtl_xor(dest,dest,&t0);
	    break;
    case CC_LE:
	    assert(dest!=&t0);
	    rtl_get_SF(dest); //E
	    rtl_get_OF(&t0);
      	    rtl_xor(dest,dest,&t0);
	    rtl_get_ZF(&t0);
	    rtl_or(dest,dest,&t0);
	    break;
    default:
	   panic("should not reach here");
  }

  if (invert) {
    rtl_xori(dest, dest, 0x1);
  }
}
