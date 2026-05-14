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


#if BX_CPU_LEVEL >= 5  //1172
Bit32u BX_CPU_C::get_cr4_allow_mask(void)
{
    Bit32u allowMask = 0;

    // CR4 bits definitions:
    //   [31-28] Reserved, Must be Zero
    //   [27]    LASS: Linear Address Separation Enable R/W
    //   [26]    Reserved, Must be Zero
    //   [25]    UINTR: User Level Interrupt Enable R/W
    //   [24]    PKS: Protection Keys Supervisor Enable R/W
    //   [23]    CET: Control Flow Enforcement R/W
    //   [22]    PKE: Protection Keys Enable R/W
    //   [21]    SMAP: Supervisor Mode Access Prevention R/W
    //   [20]    SMEP: Supervisor Mode Execution Protection R/W
    //   [19]    Reserved, Must be Zero
    //   [18]    OSXSAVE: Operating System XSAVE Support R/W
    //   [17]    PCIDE: PCID Support R/W
    //   [16]    FSGSBASE: FS/GS BASE access R/W
    //   [15]    Reserved, Must be Zero
    //   [14]    SMXE: SMX Extensions R/W
    //   [13]    VMXE: VMX Extensions R/W
    //   [12]    LA57, 57-bit Linear Address and 5-level paging
    //   [11]    UMIP: User Mode Instruction Prevention R/W
    //   [10]    OSXMMEXCPT: Operating System Unmasked Exception Support R/W
    //   [9]     OSFXSR: Operating System FXSAVE/FXRSTOR Support R/W
    //   [8]     PCE: Performance-Monitoring Counter Enable R/W
    //   [7]     PGE: Page-Global Enable R/W
    //   [6]     MCE: Machine Check Enable R/W
    //   [5]     PAE: Physical-Address Extension R/W
    //   [4]     PSE: Page Size Extensions R/W
    //   [3]     DE: Debugging Extensions R/W
    //   [2]     TSD: Time Stamp Disable R/W
    //   [1]     PVI: Protected-Mode Virtual Interrupts R/W
    //   [0]     VME: Virtual-8086 Mode Extensions R/W

    /* VME */
    if (is_cpu_extension_supported(BX_ISA_VME))
        allowMask |= BX_CR4_VME_MASK | BX_CR4_PVI_MASK;

    if (is_cpu_extension_supported(BX_ISA_PENTIUM))
        allowMask |= BX_CR4_TSD_MASK;

    if (is_cpu_extension_supported(BX_ISA_DEBUG_EXTENSIONS))
        allowMask |= BX_CR4_DE_MASK;

    if (is_cpu_extension_supported(BX_ISA_PSE))
        allowMask |= BX_CR4_PSE_MASK;

#if BX_CPU_LEVEL >= 6
    if (is_cpu_extension_supported(BX_ISA_PAE))
        allowMask |= BX_CR4_PAE_MASK;
#endif

    // NOTE: exception 18 (#MC) never appears in Bochs
    allowMask |= BX_CR4_MCE_MASK;

#if BX_CPU_LEVEL >= 6
    if (is_cpu_extension_supported(BX_ISA_PGE))
        allowMask |= BX_CR4_PGE_MASK;

    allowMask |= BX_CR4_PCE_MASK;

    /* OSFXSR */
    if (is_cpu_extension_supported(BX_ISA_SSE))
        allowMask |= BX_CR4_OSFXSR_MASK;

    /* OSXMMEXCPT */
    if (is_cpu_extension_supported(BX_ISA_SSE))
        allowMask |= BX_CR4_OSXMMEXCPT_MASK;

#if BX_SUPPORT_VMX
    if (is_cpu_extension_supported(BX_ISA_VMX))
        allowMask |= BX_CR4_VMXE_MASK;
#endif

    if (is_cpu_extension_supported(BX_ISA_SMX))
        allowMask |= BX_CR4_SMXE_MASK;

#if BX_SUPPORT_X86_64
    if (is_cpu_extension_supported(BX_ISA_PCID))
        allowMask |= BX_CR4_PCIDE_MASK;

    if (is_cpu_extension_supported(BX_ISA_FSGSBASE))
        allowMask |= BX_CR4_FSGSBASE_MASK;
#endif

    /* OSXSAVE */
    if (is_cpu_extension_supported(BX_ISA_XSAVE))
        allowMask |= BX_CR4_OSXSAVE_MASK;

    if (is_cpu_extension_supported(BX_ISA_SMEP))
        allowMask |= BX_CR4_SMEP_MASK;

    if (is_cpu_extension_supported(BX_ISA_SMAP))
        allowMask |= BX_CR4_SMAP_MASK;

#if BX_SUPPORT_PKEYS
    if (is_cpu_extension_supported(BX_ISA_PKU))
        allowMask |= BX_CR4_PKE_MASK;
#endif

    if (is_cpu_extension_supported(BX_ISA_UMIP))
        allowMask |= BX_CR4_UMIP_MASK;

#if BX_SUPPORT_X86_64
    if (is_cpu_extension_supported(BX_ISA_LA57))
        allowMask |= BX_CR4_LA57_MASK;
#endif

#if BX_SUPPORT_CET
    if (is_cpu_extension_supported(BX_ISA_CET))
        allowMask |= BX_CR4_CET_MASK;
#endif

#if BX_SUPPORT_PKEYS
    if (is_cpu_extension_supported(BX_ISA_PKS))
        allowMask |= BX_CR4_PKS_MASK;
#endif

#if BX_SUPPORT_UINTR
    if (is_cpu_extension_supported(BX_ISA_UINTR))
        allowMask |= BX_CR4_UINTR_MASK;
#endif

    if (is_cpu_extension_supported(BX_ISA_LASS))
        allowMask |= BX_CR4_LASS_MASK;
#endif

    return allowMask;
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

#endif // BX_CPU_LEVEL >= 5  //1421
#if BX_X86_DEBUGGER //1601

bool BX_CPU_C::hwbreakpoint_check(bx_address laddr, unsigned opa, unsigned opb)
{
    laddr = LPFOf(laddr);

    Bit32u dr_op[4];

    dr_op[0] = BX_CPU_THIS_PTR dr7.get_R_W0();
    dr_op[1] = BX_CPU_THIS_PTR dr7.get_R_W1();
    dr_op[2] = BX_CPU_THIS_PTR dr7.get_R_W2();
    dr_op[3] = BX_CPU_THIS_PTR dr7.get_R_W3();

    for (int n = 0; n < 4; n++) {
        if ((dr_op[n] == opa || dr_op[n] == opb) && laddr == LPFOf(BX_CPU_THIS_PTR dr[n])) {
            return true;
        }
    }

    return false;
}

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

#if BX_CPU_LEVEL >= 5
void BX_CPU_C::iobreakpoint_match(unsigned port, unsigned len)
{
    // Only compare debug registers if any breakpoints are enabled
    if (BX_CPU_THIS_PTR cr4.get_DE() && BX_CPU_THIS_PTR dr7.get_bp_enabled())
    {
        Bit32u dr6_bits = hwdebug_compare(port, len, BX_HWDebugIO, BX_HWDebugIO);
        if (dr6_bits) {
            BX_CPU_THIS_PTR debug_trap |= dr6_bits;
            if (BX_CPU_THIS_PTR debug_trap & BX_DEBUG_TRAP_HIT) {
                //BX_ERROR(("#DB: I/O breakpoint hit - report debug trap on next instruction"));
                BX_CPU_THIS_PTR async_event = 1;
            }
        }
    }
}
#endif

#endif //1723
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

#if BX_CPU_LEVEL >= 5

Bit32u BX_CPU_C::get_efer_allow_mask(void)
{  //1870
    Bit32u efer_allowed_mask = 0;

    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_NX))
        efer_allowed_mask |= BX_EFER_NXE_MASK;
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SYSCALL_SYSRET_LEGACY))
        efer_allowed_mask |= BX_EFER_SCE_MASK;
#if BX_SUPPORT_X86_64
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_LONG_MODE)) {
        efer_allowed_mask |= (BX_EFER_SCE_MASK | BX_EFER_LME_MASK | BX_EFER_LMA_MASK);
        if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_FFXSR))
            efer_allowed_mask |= BX_EFER_FFXSR_MASK;
#if BX_SUPPORT_SVM
        if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SVM))
            efer_allowed_mask |= BX_EFER_SVME_MASK;
#endif
        if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_TCE))
            efer_allowed_mask |= BX_EFER_TCE_MASK;
    }
#endif

    return efer_allowed_mask;
}

#endif

Bit32u BX_CPU_C::get_xcr0_allow_mask(void)
{  //1897
    Bit32u allowMask = BX_XCR0_FPU_MASK | BX_XCR0_SSE_MASK;
#if BX_SUPPORT_AVX
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX))
        allowMask |= BX_XCR0_YMM_MASK;
#if BX_SUPPORT_EVEX
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX512))
        allowMask |= BX_XCR0_OPMASK_MASK | BX_XCR0_ZMM_HI256_MASK | BX_XCR0_HI_ZMM_MASK;
#endif
#endif // BX_SUPPORT_AVX
#if BX_SUPPORT_PKEYS
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_PKU))
        allowMask |= BX_XCR0_PKRU_MASK;
#endif
#if BX_SUPPORT_AMX
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AMX))
        allowMask |= BX_XCR0_XTILE_BITS_MASK;
#endif
    return allowMask;
}

Bit32u BX_CPU_C::get_ia32_xss_allow_mask(void)
{
    Bit32u ia32_xss_support_mask = 0;
#if BX_SUPPORT_CET
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET))
        ia32_xss_support_mask |= BX_XCR0_CET_U_MASK | BX_XCR0_CET_S_MASK;
#endif
#if BX_SUPPORT_UINTR
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_UINTR))
        ia32_xss_support_mask |= BX_XCR0_UINTR_MASK;
#endif
    return ia32_xss_support_mask;
}
#endif