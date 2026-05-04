#pragma once
enum BxSegregs { //40ÐÐ
    BX_SEG_REG_CS = 1,
};

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
