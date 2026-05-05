#pragma once
#ifndef BX_OPCODES_ENUM
#define BX_OPCODES_ENUM

enum {
#define bx_define_opcode(a, b, c, d, e, f, s1, s2, s3, s4, g) a,
#include "ia_opcodes.def"
	BX_IA_LAST
};
#undef  bx_define_opcode

#endif