#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#if BX_CPU_LEVEL >= 6  //1725-1933
XSaveRestoreStateHelper xsave_restore[xcr0_t::BX_XCR0_LAST] = { {0, 0, NULL, NULL, NULL, NULL} };

void BX_CPU_C::xsave_xrestor_init(void)
{
	// XCR0[0]: x87 state
	// XCR0[0]: x87 state
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].len = XSAVE_FPU_STATE_LEN;
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].offset = XSAVE_FPU_STATE_OFFSET;
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].xstate_in_use_method = &BX_CPU_C::xsave_x87_state_xinuse;
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].xsave_method = &BX_CPU_C::xsave_x87_state;
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].xrstor_method = &BX_CPU_C::xrstor_x87_state;
	xsave_restore[xcr0_t::BX_XCR0_FPU_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_x87_state;

    // XCR0[1]: SSE state
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].len = XSAVE_SSE_STATE_LEN;
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].offset = XSAVE_SSE_STATE_OFFSET;
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].xstate_in_use_method = &BX_CPU_C::xsave_sse_state_xinuse;
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].xsave_method = &BX_CPU_C::xsave_sse_state;
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].xrstor_method = &BX_CPU_C::xrstor_sse_state;
    xsave_restore[xcr0_t::BX_XCR0_SSE_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_sse_state;

#if BX_SUPPORT_AVX
    // XCR0[2]: YMM state
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX)) {
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].len = XSAVE_YMM_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].offset = XSAVE_YMM_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].xstate_in_use_method = &BX_CPU_C::xsave_ymm_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].xsave_method = &BX_CPU_C::xsave_ymm_state;
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].xrstor_method = &BX_CPU_C::xrstor_ymm_state;
        xsave_restore[xcr0_t::BX_XCR0_YMM_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_ymm_state;
    }
#endif

    // XCR0[3]: BNDREGS state (not implemented, deprecated)
    // XCR0[4]: BNDCFG state (not implemented, deprecated)

#if BX_SUPPORT_EVEX
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX512)) {
        // XCR0[5]: OPMASK state
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].len = XSAVE_OPMASK_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].offset = XSAVE_OPMASK_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].xstate_in_use_method = &BX_CPU_C::xsave_opmask_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].xsave_method = &BX_CPU_C::xsave_opmask_state;
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].xrstor_method = &BX_CPU_C::xrstor_opmask_state;
        xsave_restore[xcr0_t::BX_XCR0_OPMASK_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_opmask_state;

        // XCR0[6]: ZMM_HI256 state
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].len = XSAVE_ZMM_HI256_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].offset = XSAVE_ZMM_HI256_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].xstate_in_use_method = &BX_CPU_C::xsave_zmm_hi256_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].xsave_method = &BX_CPU_C::xsave_zmm_hi256_state;
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].xrstor_method = &BX_CPU_C::xrstor_zmm_hi256_state;
        xsave_restore[xcr0_t::BX_XCR0_ZMM_HI256_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_zmm_hi256_state;

        // XCR0[7]: ZMM_HI state
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].len = XSAVE_HI_ZMM_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].offset = XSAVE_HI_ZMM_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].xstate_in_use_method = &BX_CPU_C::xsave_hi_zmm_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].xsave_method = &BX_CPU_C::xsave_hi_zmm_state;
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].xrstor_method = &BX_CPU_C::xrstor_hi_zmm_state;
        xsave_restore[xcr0_t::BX_XCR0_HI_ZMM_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_hi_zmm_state;
    }
#endif

    // XCR0[8]: Processor Trace state (not implemented)

#if BX_SUPPORT_PKEYS
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_PKU)) {
        // XCR0[9]: PKRU state
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].len = XSAVE_PKRU_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].offset = XSAVE_PKRU_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].xstate_in_use_method = &BX_CPU_C::xsave_pkru_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].xsave_method = &BX_CPU_C::xsave_pkru_state;
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].xrstor_method = &BX_CPU_C::xrstor_pkru_state;
        xsave_restore[xcr0_t::BX_XCR0_PKRU_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_pkru_state;
    }
#endif

    // XCR0[10]: PASID state (not implemented, IA32_XSS only)

#if BX_SUPPORT_CET
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET)) {
        // XCR0[11]: CET User State
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].len = XSAVE_CET_U_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].offset = 0;    // IA32_XSS only
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].xstate_in_use_method = &BX_CPU_C::xsave_cet_u_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].xsave_method = &BX_CPU_C::xsave_cet_u_state;
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].xrstor_method = &BX_CPU_C::xrstor_cet_u_state;
        xsave_restore[xcr0_t::BX_XCR0_CET_U_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_cet_u_state;

        // XCR0[12]: CET Supervisor State
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].len = XSAVE_CET_S_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].offset = 0;    // IA32_XSS only
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].xstate_in_use_method = &BX_CPU_C::xsave_cet_s_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].xsave_method = &BX_CPU_C::xsave_cet_s_state;
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].xrstor_method = &BX_CPU_C::xrstor_cet_s_state;
        xsave_restore[xcr0_t::BX_XCR0_CET_S_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_cet_s_state;
    }
#endif

    // XCR0[13]: HDC state (not implemented, IA32_XSS only)

#if BX_SUPPORT_UINTR
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_UINTR)) {
        // XCR0[14]: UINTR State
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].len = XSAVE_UINTR_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].offset = 0;    // IA32_XSS only
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].xstate_in_use_method = &BX_CPU_C::xsave_uintr_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].xsave_method = &BX_CPU_C::xsave_uintr_state;
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].xrstor_method = &BX_CPU_C::xrstor_uintr_state;
        xsave_restore[xcr0_t::BX_XCR0_UINTR_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_uintr_state;
    }
#endif

    // XCR0[15]: LBR state (not implemented)
    // XCR0[16]: HWP state (not implemented)

    // XCR0[17]: AMX XTILECFG state
    // XCR0[18]: AMX XTILEDATA state
#if BX_SUPPORT_AMX
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AMX)) {
        // XCR0[17]: AMX XTILECFG state
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].len = XSAVE_XTILECFG_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].offset = XSAVE_XTILECFG_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].xstate_in_use_method = &BX_CPU_C::xsave_tilecfg_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].xsave_method = &BX_CPU_C::xsave_tilecfg_state;
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].xrstor_method = &BX_CPU_C::xrstor_tilecfg_state;
        xsave_restore[xcr0_t::BX_XCR0_XTILECFG_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_tilecfg_state;

        // XCR0[18]: AMX XTILEDATA state
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].len = XSAVE_XTILEDATA_STATE_LEN;
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].offset = XSAVE_XTILEDATA_STATE_OFFSET;
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].xstate_in_use_method = &BX_CPU_C::xsave_tiledata_state_xinuse;
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].xsave_method = &BX_CPU_C::xsave_tiledata_state;
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].xrstor_method = &BX_CPU_C::xrstor_tiledata_state;
        xsave_restore[xcr0_t::BX_XCR0_XTILEDATA_BIT].xrstor_init_method = &BX_CPU_C::xrstor_init_tiledata_state;
    }
#endif

    // XCR0[19]: APX state (not implemented)
}
#endif