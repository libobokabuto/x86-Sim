#pragma once
struct bx_cr0_t {
#define IMPLEMENT_CRREG_ACCESSORS(name, bitnum)            \
  BX_CPP_INLINE bool get_##name() const {               \
    return 1 & (val32 >> bitnum);                          \
  }                                                        \
  BX_CPP_INLINE void set_##name(Bit8u val) {               \
    val32 = (val32 & ~(1<<bitnum)) | ((!!val) << bitnum);  \
  }
};
struct bx_efer_t {
    Bit32u val32;
    
#if BX_SUPPORT_X86_64
    IMPLEMENT_CRREG_ACCESSORS(LME, 8);
    IMPLEMENT_CRREG_ACCESSORS(LMA, 10);
#endif
    


    BX_CPP_INLINE Bit32u get32() const { return val32; }
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val; }
};
#if BX_CPU_LEVEL >= 6

const unsigned XSAVE_FPU_STATE_LEN = 160; //254
const unsigned XSAVE_SSE_STATE_LEN = 256;
const unsigned XSAVE_YMM_STATE_LEN = 256;
const unsigned XSAVE_OPMASK_STATE_LEN = 64;
const unsigned XSAVE_ZMM_HI256_STATE_LEN = 512;
const unsigned XSAVE_HI_ZMM_STATE_LEN = 1024;
const unsigned XSAVE_PT_STATE_LEN = 128;
const unsigned XSAVE_PKRU_STATE_LEN = 8;
const unsigned XSAVE_PASID_STATE_LEN = 8;
const unsigned XSAVE_CET_U_STATE_LEN = 16;
const unsigned XSAVE_CET_S_STATE_LEN = 24;
const unsigned XSAVE_HDC_STATE_LEN = 8;
const unsigned XSAVE_UINTR_STATE_LEN = 48;
const unsigned XSAVE_LBR_STATE_LEN = 808;
const unsigned XSAVE_HWP_STATE_LEN = 8;
const unsigned XSAVE_XTILECFG_STATE_LEN = 64;
const unsigned XSAVE_XTILEDATA_STATE_LEN = 8192;
const unsigned XSAVE_APX_STATE_LEN = 128;

const unsigned XSAVE_FPU_STATE_OFFSET = 0; //273
const unsigned XSAVE_SSE_STATE_OFFSET = 160;
const unsigned XSAVE_YMM_STATE_OFFSET = 576;
const unsigned XSAVE_OPMASK_STATE_OFFSET = 1088;
const unsigned XSAVE_ZMM_HI256_STATE_OFFSET = 1152;
const unsigned XSAVE_HI_ZMM_STATE_OFFSET = 1664;
const unsigned XSAVE_PKRU_STATE_OFFSET = 2688;
const unsigned XSAVE_XTILECFG_STATE_OFFSET = 2752;
const unsigned XSAVE_XTILEDATA_STATE_OFFSET = 2816;
const unsigned XSAVE_APX_STATE_OFFSET = 960;

struct xcr0_t {
    Bit32u  val32; // 32bit value of register

    enum {
        BX_XCR0_FPU_BIT = 0,
        BX_XCR0_SSE_BIT = 1,
        BX_XCR0_YMM_BIT = 2,
        BX_XCR0_BNDREGS_BIT = 3, // not implemented, deprecated
        BX_XCR0_BNDCFG_BIT = 4,  // not implemented, deprecated
        BX_XCR0_OPMASK_BIT = 5,
        BX_XCR0_ZMM_HI256_BIT = 6,
        BX_XCR0_HI_ZMM_BIT = 7,
        BX_XCR0_PT_BIT = 8,      // not implemented yet
        BX_XCR0_PKRU_BIT = 9,
        BX_XCR0_PASID_BIT = 10,  // not implemented yet
        BX_XCR0_CET_U_BIT = 11,
        BX_XCR0_CET_S_BIT = 12,
        BX_XCR0_HDC_BIT = 13,    // not implemented yet
        BX_XCR0_UINTR_BIT = 14,
        BX_XCR0_LBR_BIT = 15,    // not implemented yet
        BX_XCR0_HWP_BIT = 16,    // not implemented yet
        BX_XCR0_XTILECFG_BIT = 17,
        BX_XCR0_XTILEDATA_BIT = 18,
        BX_XCR0_APX_BIT = 19,
        BX_XCR0_LAST // make sure it is < 32
    };

};

#if BX_USE_CPU_SMF  //358
typedef bool (*XSaveStateInUsePtr_tR)(void);
typedef void (*XSavePtr_tR)(bxInstruction_c* i, bx_address offset);
typedef void (*XRestorPtr_tR)(bxInstruction_c* i, bx_address offset);
typedef void (*XRestorInitPtr_tR)(void);
#else
typedef bool (BX_CPU_C::* XSaveStateInUsePtr_tR)(void);
typedef void (BX_CPU_C::* XSavePtr_tR)(bxInstruction_c* i, bx_address offset);
typedef void (BX_CPU_C::* XRestorPtr_tR)(bxInstruction_c* i, bx_address offset);
typedef void (BX_CPU_C::* XRestorInitPtr_tR)(void);
#endif


struct XSaveRestoreStateHelper {
	unsigned len;
	unsigned offset;
	XSaveStateInUsePtr_tR xstate_in_use_method;
	XSavePtr_tR xsave_method;
	XRestorPtr_tR xrstor_method;
	XRestorInitPtr_tR xrstor_init_method;
};

#if BX_CPU_LEVEL >= 5  //384

typedef struct msr {
}MSR;
#endif
#endif