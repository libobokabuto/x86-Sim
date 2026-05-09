#pragma once
struct bx_cr0_t {//39
    Bit32u  val32;
#define IMPLEMENT_CRREG_ACCESSORS(name, bitnum)            \
  BX_CPP_INLINE bool get_##name() const {               \
    return 1 & (val32 >> bitnum);                          \
  }                                                        \
  BX_CPP_INLINE void set_##name(Bit8u val) {               \
    val32 = (val32 & ~(1<<bitnum)) | ((!!val) << bitnum);  \
  }

    IMPLEMENT_CRREG_ACCESSORS(PE, 0);
    IMPLEMENT_CRREG_ACCESSORS(MP, 1);
    IMPLEMENT_CRREG_ACCESSORS(EM, 2);
    IMPLEMENT_CRREG_ACCESSORS(TS, 3);
#if BX_CPU_LEVEL >= 4
    IMPLEMENT_CRREG_ACCESSORS(ET, 4);
    IMPLEMENT_CRREG_ACCESSORS(NE, 5);
    IMPLEMENT_CRREG_ACCESSORS(WP, 16);
    IMPLEMENT_CRREG_ACCESSORS(AM, 18);
    IMPLEMENT_CRREG_ACCESSORS(NW, 29);
    IMPLEMENT_CRREG_ACCESSORS(CD, 30);
#endif
    IMPLEMENT_CRREG_ACCESSORS(PG, 31);

    BX_CPP_INLINE Bit32u get32() const { return val32; }//86
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val | 0x10; }

}; //87

struct bx_cr4_t {
    Bit32u  val32;
    IMPLEMENT_CRREG_ACCESSORS(VME, 0);
    IMPLEMENT_CRREG_ACCESSORS(PVI, 1);
    IMPLEMENT_CRREG_ACCESSORS(TSD, 2);
    IMPLEMENT_CRREG_ACCESSORS(DE, 3);
    IMPLEMENT_CRREG_ACCESSORS(PSE, 4);
    IMPLEMENT_CRREG_ACCESSORS(PAE, 5);
    IMPLEMENT_CRREG_ACCESSORS(MCE, 6);
    IMPLEMENT_CRREG_ACCESSORS(PGE, 7);
    IMPLEMENT_CRREG_ACCESSORS(PCE, 8);
    IMPLEMENT_CRREG_ACCESSORS(OSFXSR, 9);
    IMPLEMENT_CRREG_ACCESSORS(OSXMMEXCPT, 10);
    IMPLEMENT_CRREG_ACCESSORS(UMIP, 11);
    IMPLEMENT_CRREG_ACCESSORS(LA57, 12);
#if BX_SUPPORT_VMX
    IMPLEMENT_CRREG_ACCESSORS(VMXE, 13);
#endif
    IMPLEMENT_CRREG_ACCESSORS(SMXE, 14);
#if BX_SUPPORT_X86_64
    IMPLEMENT_CRREG_ACCESSORS(FSGSBASE, 16);
#endif
    IMPLEMENT_CRREG_ACCESSORS(PCIDE, 17);
    IMPLEMENT_CRREG_ACCESSORS(OSXSAVE, 18);
    IMPLEMENT_CRREG_ACCESSORS(KEYLOCKER, 19);
    IMPLEMENT_CRREG_ACCESSORS(SMEP, 20);
    IMPLEMENT_CRREG_ACCESSORS(SMAP, 21);
    IMPLEMENT_CRREG_ACCESSORS(PKE, 22);
    IMPLEMENT_CRREG_ACCESSORS(CET, 23);
    IMPLEMENT_CRREG_ACCESSORS(PKS, 24);
    IMPLEMENT_CRREG_ACCESSORS(UINTR, 25);
    IMPLEMENT_CRREG_ACCESSORS(LASS, 27);
    IMPLEMENT_CRREG_ACCESSORS(LAM_SUPERVISOR, 28);
    BX_CPP_INLINE Bit32u get32() const { return val32; }
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val; }
};//156

struct bx_dr6_t {
    Bit32u val32; // 32bit value of register

    IMPLEMENT_CRREG_ACCESSORS(B0, 0);
    IMPLEMENT_CRREG_ACCESSORS(B1, 1);
    IMPLEMENT_CRREG_ACCESSORS(B2, 2);
    IMPLEMENT_CRREG_ACCESSORS(B3, 3);

#define BX_DEBUG_TRAP_HIT             (1 << 12)
#define BX_DEBUG_DR_ACCESS_BIT        (1 << 13)
#define BX_DEBUG_SINGLE_STEP_BIT      (1 << 14)
#define BX_DEBUG_TRAP_TASK_SWITCH_BIT (1 << 15)

    IMPLEMENT_CRREG_ACCESSORS(BD, 13);
    IMPLEMENT_CRREG_ACCESSORS(BS, 14);
    IMPLEMENT_CRREG_ACCESSORS(BT, 15);

    BX_CPP_INLINE Bit32u get32() const { return val32; }
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val; }
};

struct bx_dr7_t {
    //183-216
    Bit32u val32; // 32bit value of register

    IMPLEMENT_CRREG_ACCESSORS(L0, 0);
    IMPLEMENT_CRREG_ACCESSORS(G0, 1);
    IMPLEMENT_CRREG_ACCESSORS(L1, 2);
    IMPLEMENT_CRREG_ACCESSORS(G1, 3);
    IMPLEMENT_CRREG_ACCESSORS(L2, 4);
    IMPLEMENT_CRREG_ACCESSORS(G2, 5);
    IMPLEMENT_CRREG_ACCESSORS(L3, 6);
    IMPLEMENT_CRREG_ACCESSORS(G3, 7);
    IMPLEMENT_CRREG_ACCESSORS(LE, 8);
    IMPLEMENT_CRREG_ACCESSORS(GE, 9);
    IMPLEMENT_CRREG_ACCESSORS(GD, 13);

#define IMPLEMENT_DRREG_ACCESSORS(name, bitmask, bitnum)      \
  int get_##name() const {                                    \
    return (val32 & (bitmask)) >> (bitnum);                   \
  }

    IMPLEMENT_DRREG_ACCESSORS(R_W0, 0x00030000, 16);
    IMPLEMENT_DRREG_ACCESSORS(LEN0, 0x000C0000, 18);
    IMPLEMENT_DRREG_ACCESSORS(R_W1, 0x00300000, 20);
    IMPLEMENT_DRREG_ACCESSORS(LEN1, 0x00C00000, 22);
    IMPLEMENT_DRREG_ACCESSORS(R_W2, 0x03000000, 24);
    IMPLEMENT_DRREG_ACCESSORS(LEN2, 0x0C000000, 26);
    IMPLEMENT_DRREG_ACCESSORS(R_W3, 0x30000000, 28);
    IMPLEMENT_DRREG_ACCESSORS(LEN3, 0xC0000000, 30);

    IMPLEMENT_DRREG_ACCESSORS(bp_enabled, 0xFF, 0);

    BX_CPP_INLINE Bit32u get32() const { return val32; }
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val; }
};
#if BX_CPU_LEVEL >= 5
#define BX_EFER_SCE_MASK       (1 <<  0)
#define BX_EFER_LME_MASK       (1 <<  8)
#define BX_EFER_LMA_MASK       (1 << 10)
#define BX_EFER_NXE_MASK       (1 << 11)
#define BX_EFER_SVME_MASK      (1 << 12)
#define BX_EFER_LMSLE_MASK     (1 << 13)
#define BX_EFER_FFXSR_MASK     (1 << 14)
#define BX_EFER_TCE_MASK       (1 << 15)

struct bx_efer_t { //229
    Bit32u val32;
    
#if BX_SUPPORT_X86_64
    IMPLEMENT_CRREG_ACCESSORS(LME, 8);
    IMPLEMENT_CRREG_ACCESSORS(LMA, 10);
#endif
    


    BX_CPP_INLINE Bit32u get32() const { return val32; }
    BX_CPP_INLINE void set32(Bit32u val) { val32 = val; }
};
#endif
#if BX_CPU_LEVEL >= 6 //251

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