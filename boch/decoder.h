#pragma once

enum x86_feature_name {
#define x86_feature(isa, feature_name) isa,
#include "features.h"
    BX_ISA_EXTENSION_LAST
};
#undef x86_feature

#define BX_ISA_EXTENSIONS_ARRAY_SIZE (5)

#if (BX_ISA_EXTENSION_LAST) >= (BX_ISA_EXTENSIONS_ARRAY_SIZE*32)
#error "ISA extensions array limit exceeded!"
#endif

enum BxSegregs { //40ÐÐ
    BX_SEG_REG_ES = 0,
    BX_SEG_REG_CS = 1,
    BX_SEG_REG_SS = 2,
    BX_SEG_REG_DS = 3,
    BX_SEG_REG_FS = 4,
    BX_SEG_REG_GS = 5,
    // NULL now has to fit in 3 bits.
    BX_SEG_REG_NULL = 7
};
#define BX_NULL_SEG_REG(seg) ((seg) == BX_SEG_REG_NULL) //51

enum BxRegs16 {
    //81
    BX_16BIT_REG_AX,
    BX_16BIT_REG_CX,
    BX_16BIT_REG_DX,
    BX_16BIT_REG_BX,
    BX_16BIT_REG_SP,
    BX_16BIT_REG_BP,
    BX_16BIT_REG_SI,
    BX_16BIT_REG_DI,
#if BX_SUPPORT_X86_64
    BX_16BIT_REG_R8,
    BX_16BIT_REG_R9,
    BX_16BIT_REG_R10,
    BX_16BIT_REG_R11,
    BX_16BIT_REG_R12,
    BX_16BIT_REG_R13,
    BX_16BIT_REG_R14,
    BX_16BIT_REG_R15,
#endif
};
#if BX_SUPPORT_X86_64
enum BxRegs64 {  //123
    BX_64BIT_REG_RAX,
    BX_64BIT_REG_RCX,
    BX_64BIT_REG_RDX,
    BX_64BIT_REG_RBX,
    BX_64BIT_REG_RSP,
    BX_64BIT_REG_RBP,
    BX_64BIT_REG_RSI,
    BX_64BIT_REG_RDI,
    BX_64BIT_REG_R8,
    BX_64BIT_REG_R9,
    BX_64BIT_REG_R10,
    BX_64BIT_REG_R11,
    BX_64BIT_REG_R12,
    BX_64BIT_REG_R13,
    BX_64BIT_REG_R14,
    BX_64BIT_REG_R15,
};
#endif
#if BX_SUPPORT_X86_64        //144ÐÐ
# define BX_GENERAL_REGISTERS 16
#else
# define BX_GENERAL_REGISTERS 8
#endif
static const unsigned BX_16BIT_REG_IP = (BX_GENERAL_REGISTERS),
                      BX_32BIT_REG_EIP = (BX_GENERAL_REGISTERS),
                      BX_64BIT_REG_RIP = (BX_GENERAL_REGISTERS);
static const unsigned BX_32BIT_REG_SSP = (BX_GENERAL_REGISTERS + 1),
                      BX_64BIT_REG_SSP = (BX_GENERAL_REGISTERS + 1);

static const unsigned BX_TMP_REGISTER = (BX_GENERAL_REGISTERS + 2);
static const unsigned BX_NIL_REGISTER = (BX_GENERAL_REGISTERS + 3);

enum bx_avx_vector_length {
    BX_NO_VL,
    BX_VL128 = 1,
    BX_VL256 = 2,
    BX_VL512 = 4,
};

#if BX_SUPPORT_EVEX
#  define BX_XMM_REGISTERS 32
#else
#  if BX_SUPPORT_X86_64
#    define BX_XMM_REGISTERS 16
#  else
#    define BX_XMM_REGISTERS 8
#  endif
#endif

static const unsigned BX_VECTOR_TMP_REGISTER = (BX_XMM_REGISTERS);
