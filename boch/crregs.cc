#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif
#if BX_SUPPORT_APIC
#include "apic.h"
#endif
void BX_CPU_C::hwbreakpoint_match(bx_address laddr, unsigned len, unsigned rw)
{
    if (BX_CPU_THIS_PTR dr7.get_bp_enabled()) {
        // Only compare debug registers if any breakpoints are enabled
        unsigned opa, opb, write = rw & 1;
        opa = BX_HWDebugMemRW; // Read or Write always compares vs 11b
        if (!write) // only compares vs 11b
            opb = opa;
        else // BX_WRITE or BX_RW; also compare vs 01b
            opb = BX_HWDebugMemW;
        Bit32u dr6_bits = hwdebug_compare(laddr, len, opa, opb);
        if (dr6_bits) {
            BX_CPU_THIS_PTR debug_trap |= dr6_bits;
            if (BX_CPU_THIS_PTR debug_trap & BX_DEBUG_TRAP_HIT) {
                //BX_ERROR(("#DB: Code/Data breakpoint hit - report debug trap on next instruction"));
                BX_CPU_THIS_PTR async_event = 1;
            }
        }
    }
}

Bit32u BX_CPU_C::hwdebug_compare(bx_address laddr_0, unsigned size,
    unsigned opa, unsigned opb)
{
    Bit32u dr7 = BX_CPU_THIS_PTR dr7.get32();

    static bx_address alignment_mask[4] =
        // 00b=1  01b=2  10b=undef(8)  11b=4
    { 0x0,   0x1,   0x7,          0x3 };

    bx_address laddr_n = laddr_0 + (size - 1);
    Bit32u dr_op[4], dr_len[4];

    // If *any* enabled breakpoints matched, then we need to
    // set status bits for *all* breakpoints, even disabled ones,
    // as long as they meet the other breakpoint criteria.
    // dr6_mask is the return value.  These bits represent the bits
    // to be OR'd into DR6 as a result of the debug event.
    Bit32u dr6_mask = 0;

    dr_len[0] = BX_CPU_THIS_PTR dr7.get_LEN0();
    dr_len[1] = BX_CPU_THIS_PTR dr7.get_LEN1();
    dr_len[2] = BX_CPU_THIS_PTR dr7.get_LEN2();
    dr_len[3] = BX_CPU_THIS_PTR dr7.get_LEN3();

    dr_op[0] = BX_CPU_THIS_PTR dr7.get_R_W0();
    dr_op[1] = BX_CPU_THIS_PTR dr7.get_R_W1();
    dr_op[2] = BX_CPU_THIS_PTR dr7.get_R_W2();
    dr_op[3] = BX_CPU_THIS_PTR dr7.get_R_W3();

    for (unsigned n = 0; n < 4; n++) {
        bx_address dr_start = BX_CPU_THIS_PTR dr[n] & ~alignment_mask[dr_len[n]];
        bx_address dr_end = dr_start + alignment_mask[dr_len[n]];

        // See if this instruction address matches any breakpoints
        if ((dr_op[n] == opa || dr_op[n] == opb) &&
            (laddr_0 <= dr_end) &&
            (laddr_n >= dr_start)) {
            dr6_mask |= (1 << n);
            // tell if breakpoint was enabled
            if (dr7 & (3 << n * 2)) {
                dr6_mask |= BX_DEBUG_TRAP_HIT;
            }
        }
    }

    return dr6_mask;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::check_CR0(bx_address cr0_val)
{  //990
    bx_cr0_t temp_cr0;

#if BX_SUPPORT_X86_64
    if (GET32H(cr0_val)) {
        //BX_ERROR(("check_CR0(): trying to set CR0 > 32 bits"));
        return false;
    }
#endif

    temp_cr0.set32((Bit32u)cr0_val);

#if BX_SUPPORT_SVM
    if (!BX_CPU_THIS_PTR in_svm_guest) // it should be fine to enter paged real mode in SVM guest
#endif
    {
        if (temp_cr0.get_PG() && !temp_cr0.get_PE()) {
            //BX_ERROR(("check_CR0(0x%08x): attempt to set CR0.PG with CR0.PE cleared !", temp_cr0.get32()));
            return false;
        }
    }

#if BX_CPU_LEVEL >= 4
    if (temp_cr0.get_NW() && !temp_cr0.get_CD()) {
        //BX_ERROR(("check_CR0(0x%08x): attempt to set CR0.NW with CR0.CD cleared !", temp_cr0.get32()));
        return false;
    }
#endif

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx) {
        if (!temp_cr0.get_NE()) {
            //BX_ERROR(("check_CR0(0x%08x): attempt to clear CR0.NE in vmx mode !", temp_cr0.get32()));
            return false;
        }
        if (!BX_CPU_THIS_PTR in_vmx_guest && !BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.UNRESTRICTED_GUEST()) {
            if (!temp_cr0.get_PE() || !temp_cr0.get_PG()) {
                //BX_ERROR(("check_CR0(0x%08x): attempt to clear CR0.PE/CR0.PG in vmx mode !", temp_cr0.get32()));
                return false;
            }
        }
    }
#endif

    return true;
}

bool BX_CPP_AttrRegparmN(1) BX_CPU_C::check_CR4(bx_address cr4_val)
{
    //1304
    // check if trying to set undefined bits
    if (cr4_val & ~((bx_address)BX_CPU_THIS_PTR cr4_suppmask)) {
        //BX_ERROR(("check_CR4(): write of 0x%08x not supported (allowMask=0x%x)", (Bit32u)cr4_val, BX_CPU_THIS_PTR cr4_suppmask));
        return false;
    }

    bx_cr4_t temp_cr4;
    temp_cr4.set32((Bit32u)cr4_val);

#if BX_SUPPORT_X86_64
    if (long_mode()) {
        if (!temp_cr4.get_PAE()) {
            //BX_ERROR(("check_CR4(): attempt to clear CR4.PAE when EFER.LMA=1"));
            return false;
        }

        if (temp_cr4.get_LA57() != BX_CPU_THIS_PTR cr4.get_LA57()) {
            //BX_ERROR(("check_CR4(): attempt to change CR4.LA57 when EFER.LMA=1"));
            return false;
        }
    }
    else {
        if (temp_cr4.get_PCIDE()) {
            //BX_ERROR(("check_CR4(): attempt to set CR4.PCIDE when EFER.LMA=0"));
            return false;
        }
    }
#endif

#if BX_SUPPORT_VMX
    if (!temp_cr4.get_VMXE()) {
        if (BX_CPU_THIS_PTR in_vmx) {
            //BX_ERROR(("check_CR4(): attempt to clear CR4.VMXE in vmx mode"));
            return false;
        }
    }
    else {
        if (BX_CPU_THIS_PTR in_smm) {
            //BX_ERROR(("check_CR4(): attempt to set CR4.VMXE in smm mode"));
            return false;
        }
    }
#endif

    return true;
}


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