#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR
#include "param_names.h"
#include "cpustats.h"

#if BX_SUPPORT_APIC
#include "apic.h"
#endif

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

BX_CPU_C::BX_CPU_C(unsigned id) : bx_cpuid(id)
#if BX_CPU_LEVEL >= 4
, cpuid(NULL)
#endif
{
	
    char name[16], logname[16];
    sprintf(name, "CPU%x", bx_cpuid);
    sprintf(logname, "cpu%x", bx_cpuid);
    //put(logname, name);

#if BX_SUPPORT_APIC
    lapic = new bx_local_apic_c(this, bx_cpuid);
#endif

    for (unsigned n = 0; n < BX_ISA_EXTENSIONS_ARRAY_SIZE; n++)
        ia_extensions_bitmask[n] = 0;

    ia_extensions_bitmask[0] = (1 << BX_ISA_386);
    if (BX_SUPPORT_FPU)
        ia_extensions_bitmask[0] |= (1 << BX_ISA_X87);

#if BX_SUPPORT_VMX
    vmx_extensions_bitmask = 0;
#endif
#if BX_SUPPORT_SVM
    svm_extensions_bitmask = 0;
#endif

    stats = NULL;

    srand(time(NULL)); // initialize random generator for RDRAND/RDSEED

}
#include <stdlib.h>
#if BX_CPU_LEVEL >= 4
#include "cpuid.h"
#include "i386.h"
#include "i486dx4.h"
#include "corei7_haswell_4770.h"
static bx_cpuid_t* cpuid_factory(BX_CPU_C* cpu)
{
    // 源码这里用 SIM->get_param_enum(BXPN_CPU_MODEL)->get()
    // 你这里不补 SIM，所以固定返回最小真实 CPUID 模型。
    return create_corei7_haswell_4770_cpuid(cpu);
}

#endif

void BX_CPU_C::initialize(void)
{
#if BX_CPU_LEVEL >= 4
    // 不走 SIM，不走 cpuid_factory，先固定一个最小可用 CPUID 模型。
    BX_CPU_THIS_PTR cpuid = cpuid_factory(this);

    if (!BX_CPU_THIS_PTR cpuid) {
        // 这里不要继续往下走，否则后面 cpuid->xxx 还是会空指针。
        return;
    }

    // 关键：把 CPUID 模型支持的 ISA feature 同步到 CPU。
    // init_FetchDecodeTables() 会依赖 ia_extensions_bitmask 判断哪些指令可用。
    BX_CPU_THIS_PTR cpuid->get_cpu_extensions(BX_CPU_THIS_PTR ia_extensions_bitmask);

#if BX_SUPPORT_VMX
    BX_CPU_THIS_PTR vmx_extensions_bitmask =
        BX_CPU_THIS_PTR cpuid->get_vmx_extensions_bitmask();
#endif

#if BX_SUPPORT_SVM
    BX_CPU_THIS_PTR svm_extensions_bitmask =
        BX_CPU_THIS_PTR cpuid->get_svm_extensions_bitmask();
#endif

    // 不补 SIM 时，这两段不要写：
    // add_remove_cpuid_features(features_to_exclude, false);
    // add_remove_cpuid_features(features_to_add, true);
    //
    BX_CPU_THIS_PTR cpuid->sanity_checks(); //也先别调，会牵出更多 CPUID helper。
#endif

    init_FetchDecodeTables();

#if BX_CPU_LEVEL >= 6
    xsave_xrestor_init();
#endif

#if BX_SUPPORT_AMX
    amx = NULL;
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AMX)) {
        amx = new AMX;
    }
#endif

#if BX_SUPPORT_SVM
    vmcb = NULL;
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SVM)) {
        vmcb = new VMCB_CACHE;
    }
#endif

#if BX_CPU_LEVEL >= 5
    init_MSRs();

#if BX_CONFIGURE_MSRS
    for (unsigned n = 0; n < BX_MSR_MAX_INDEX; n++) {
        BX_CPU_THIS_PTR msrs[n] = 0;
    }
#endif
#endif

    init_SMRAM();

#if BX_SUPPORT_VMX
    init_VMCS();
#endif

    init_statistics();
}

void BX_CPU_C::init_statistics(void)
{
    //253
}

void BX_CPU_C::reset(unsigned source)
{  //856-1269
    unsigned n;

    if (source == BX_RESET_HARDWARE) {
        //BX_INFO(("cpu hardware reset"));
    }
    else if (source == BX_RESET_SOFTWARE) {
        //BX_INFO(("cpu software reset"));
    }
    else
    {
        //BX_INFO(("cpu reset"));
    }

    for (n = 0; n < BX_GENERAL_REGISTERS; n++)
        BX_WRITE_32BIT_REGZ(n, 0);

    //BX_WRITE_32BIT_REGZ(BX_32BIT_REG_EDX, get_cpu_version_information());

      // initialize NIL register
    BX_WRITE_32BIT_REGZ(BX_NIL_REGISTER, 0);

    BX_CPU_THIS_PTR eflags = 0x2; // Bit1 is always set
    // clear lazy flags state to satisfy Valgrind uninitialized variables checker
    memset(&BX_CPU_THIS_PTR oszapc, 0, sizeof(BX_CPU_THIS_PTR oszapc));
    clearEFlagsOSZAPC();	        // update lazy flags state

    if (source == BX_RESET_HARDWARE)
        BX_CPU_THIS_PTR icount = 0;
    BX_CPU_THIS_PTR icount_last_sync = BX_CPU_THIS_PTR icount;

    BX_CPU_THIS_PTR trace_empty_instr = false;
    BX_CPU_THIS_PTR trace_empty_instr_line = 0;
    BX_CPU_THIS_PTR trace_empty_instr_func = NULL;

    BX_CPU_THIS_PTR inhibit_mask = 0;
    BX_CPU_THIS_PTR inhibit_icount = 0;

    BX_CPU_THIS_PTR activity_state = BX_ACTIVITY_STATE_ACTIVE;
    BX_CPU_THIS_PTR debug_trap = 0;

    /* instruction pointer */
#if BX_CPU_LEVEL < 2
    BX_CPU_THIS_PTR prev_rip = EIP = 0x00000000;
#else /* from 286 up */
    BX_CPU_THIS_PTR prev_rip = RIP = 0x0000FFF0;
#endif

    /* CS (Code Segment) and descriptor cache */
    /* Note: on a real cpu, CS initially points to upper memory.  After
     * the 1st jump, the descriptor base is zero'd out.  Since I'm just
     * going to jump to my BIOS, I don't need to do this.
     * For future reference:
     *   processor  cs.selector   cs.base    cs.limit    EIP
     *        8086    FFFF          FFFF0        FFFF   0000
     *        286     F000         FF0000        FFFF   FFF0
     *        386+    F000       FFFF0000        FFFF   FFF0
     */
    parse_selector(0xf000,
        &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector);

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1;  /* data/code segment */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = BX_DATA_READ_WRITE_ACCESSED;

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base = 0xFFFF0000;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xFFFF;

#if BX_CPU_LEVEL >= 3
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g = 0; /* byte granular */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l = 0; /* 16bit default size */
#endif
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl = 0;
#endif

    flushICaches();

    /* DS (Data Segment) and descriptor cache */
    parse_selector(0x0000,
        &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector);

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid = SegValidCache | SegAccessROK | SegAccessWOK;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.p = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.dpl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.segment = 1; /* data/code segment */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.type = BX_DATA_READ_WRITE_ACCESSED;

    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base = 0x00000000;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled = 0xFFFF;
#if BX_CPU_LEVEL >= 3
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.avl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g = 0; /* byte granular */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b = 0; /* 16bit default size */
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.l = 0; /* 16bit default size */
#endif
#endif

    // use DS segment as template for the others
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
#if BX_CPU_LEVEL >= 3
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS] = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
#endif

    /* GDTR (Global Descriptor Table Register) */
    BX_CPU_THIS_PTR gdtr.base = 0x00000000;
    BX_CPU_THIS_PTR gdtr.limit = 0xFFFF;

    /* IDTR (Interrupt Descriptor Table Register) */
    BX_CPU_THIS_PTR idtr.base = 0x00000000;
    BX_CPU_THIS_PTR idtr.limit = 0xFFFF; /* always byte granular */

    /* LDTR (Local Descriptor Table Register) */
    BX_CPU_THIS_PTR ldtr.selector.value = 0x0000;
    BX_CPU_THIS_PTR ldtr.selector.index = 0x0000;
    BX_CPU_THIS_PTR ldtr.selector.ti = 0;
    BX_CPU_THIS_PTR ldtr.selector.rpl = 0;

    BX_CPU_THIS_PTR ldtr.cache.valid = SegValidCache; /* valid */
    BX_CPU_THIS_PTR ldtr.cache.p = 1; /* present */
    BX_CPU_THIS_PTR ldtr.cache.dpl = 0; /* field not used */
    BX_CPU_THIS_PTR ldtr.cache.segment = 0; /* system segment */
    BX_CPU_THIS_PTR ldtr.cache.type = BX_SYS_SEGMENT_LDT;
    BX_CPU_THIS_PTR ldtr.cache.u.segment.base = 0x00000000;
    BX_CPU_THIS_PTR ldtr.cache.u.segment.limit_scaled = 0xFFFF;
    BX_CPU_THIS_PTR ldtr.cache.u.segment.avl = 0;
    BX_CPU_THIS_PTR ldtr.cache.u.segment.g = 0;  /* byte granular */

    /* TR (Task Register) */
    BX_CPU_THIS_PTR tr.selector.value = 0x0000;
    BX_CPU_THIS_PTR tr.selector.index = 0x0000; /* undefined */
    BX_CPU_THIS_PTR tr.selector.ti = 0;
    BX_CPU_THIS_PTR tr.selector.rpl = 0;

    BX_CPU_THIS_PTR tr.cache.valid = SegValidCache; /* valid */
    BX_CPU_THIS_PTR tr.cache.p = 1; /* present */
    BX_CPU_THIS_PTR tr.cache.dpl = 0; /* field not used */
    BX_CPU_THIS_PTR tr.cache.segment = 0; /* system segment */
    BX_CPU_THIS_PTR tr.cache.type = BX_SYS_SEGMENT_BUSY_386_TSS;
    BX_CPU_THIS_PTR tr.cache.u.segment.base = 0x00000000;
    BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled = 0xFFFF;
    BX_CPU_THIS_PTR tr.cache.u.segment.avl = 0;
    BX_CPU_THIS_PTR tr.cache.u.segment.g = 0;  /* byte granular */

    BX_CPU_THIS_PTR cpu_mode = BX_MODE_IA32_REAL;

    // DR0 - DR7 (Debug Registers)
#if BX_CPU_LEVEL >= 3
    for (n = 0; n < 4; n++)
        BX_CPU_THIS_PTR dr[n] = 0;
#endif

#if BX_CPU_LEVEL >= 5
    BX_CPU_THIS_PTR dr6.val32 = 0xFFFF0FF0;
#else
    BX_CPU_THIS_PTR dr6.val32 = 0xFFFF1FF0;
#endif
    BX_CPU_THIS_PTR dr7.val32 = 0x00000400;

    BX_CPU_THIS_PTR in_smm = false;

    BX_CPU_THIS_PTR pending_event = 0;
    BX_CPU_THIS_PTR event_mask = 0;

    if (source == BX_RESET_HARDWARE) {
        BX_CPU_THIS_PTR smbase = 0x30000; // do not change SMBASE on INIT
    }

#if BX_SUPPORT_FPU
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_X87))
        BX_CPU_THIS_PTR cr0.set32(0x60000010);
    else
#else
    BX_CPU_THIS_PTR cr0.set32(0x60000000);
#endif

    // handle reserved bits
#if BX_CPU_LEVEL == 3
  // reserved bits all set to 1 on 386
    BX_CPU_THIS_PTR cr0.val32 |= 0x7fffffe0;
#endif

#if BX_CPU_LEVEL >= 3
    BX_CPU_THIS_PTR cr2 = 0;
    BX_CPU_THIS_PTR cr3 = 0;
#endif

#if BX_CPU_LEVEL >= 5
    BX_CPU_THIS_PTR cr4.set32(0);
    BX_CPU_THIS_PTR cr4_suppmask = get_cr4_allow_mask();
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR linaddr_width = 48;
#endif
#endif

#if BX_CPU_LEVEL >= 6
    if (source == BX_RESET_HARDWARE) {
        BX_CPU_THIS_PTR xcr0.set32(0x1);
    }
    BX_CPU_THIS_PTR xcr0_suppmask = get_xcr0_allow_mask();
    BX_CPU_THIS_PTR ia32_xss_suppmask = get_ia32_xss_allow_mask();

    BX_CPU_THIS_PTR msr.ia32_xss = 0;

#if BX_SUPPORT_MONITOR_MWAIT
    BX_CPU_THIS_PTR msr.ia32_umwait_ctrl = 0;
#endif

#if BX_SUPPORT_SVM
    BX_CPU_THIS_PTR msr.svm_hsave_pa = 0;
    BX_CPU_THIS_PTR msr.svm_vm_cr = 0;     // enable SVME if was disabled, clear LOCK bit
#endif

#if BX_SUPPORT_CET
    BX_CPU_THIS_PTR msr.ia32_interrupt_ssp_table = 0;
    BX_CPU_THIS_PTR msr.ia32_cet_control[0] = BX_CPU_THIS_PTR msr.ia32_cet_control[1] = 0;
    for (n = 0; n < 4; n++)
        BX_CPU_THIS_PTR msr.ia32_pl_ssp[n] = 0;
    SSP = 0;
#endif
#endif // BX_CPU_LEVEL >= 6

#if BX_SUPPORT_UINTR
    memset(&BX_CPU_THIS_PTR uintr, 0, sizeof(BX_CPU_THIS_PTR uintr));
#endif

#if BX_CPU_LEVEL >= 5
    BX_CPU_THIS_PTR msr.ia32_spec_ctrl = 0;

    /* initialise MSR registers to defaults */
#if BX_SUPPORT_APIC
  /* APIC Address, APIC enabled and BSP is default, we'll fill in the rest later */
    BX_CPU_THIS_PTR msr.apicbase = BX_LAPIC_BASE_ADDR;
    BX_CPU_THIS_PTR lapic->reset(source);
    BX_CPU_THIS_PTR msr.apicbase |= 0x900;
    BX_CPU_THIS_PTR lapic->set_base(BX_CPU_THIS_PTR msr.apicbase);
#if BX_CPU_LEVEL >= 6
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_XAPIC_EXT))
        BX_CPU_THIS_PTR lapic->enable_xapic_extensions();
#endif
#endif

    BX_CPU_THIS_PTR efer.set32(0);
    BX_CPU_THIS_PTR efer_suppmask = get_efer_allow_mask();

    BX_CPU_THIS_PTR msr.star = 0;
#if BX_SUPPORT_X86_64
    if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_LONG_MODE)) {
        if (source == BX_RESET_HARDWARE) {
            BX_CPU_THIS_PTR msr.lstar = 0;
            BX_CPU_THIS_PTR msr.cstar = 0;
        }
        BX_CPU_THIS_PTR msr.fmask = 0x00020200;
        BX_CPU_THIS_PTR msr.kernelgsbase = 0;
        if (source == BX_RESET_HARDWARE) {
            BX_CPU_THIS_PTR msr.tsc_aux = 0;
        }
    }
#endif

#if BX_SUPPORT_VMX || BX_SUPPORT_SVM
    BX_CPU_THIS_PTR tsc_offset = 0;
#endif
    if (source == BX_RESET_HARDWARE) {
        BX_CPU_THIS_PTR set_TSC(0); // do not change TSC on INIT
    }
#endif // BX_CPU_LEVEL >= 5

    if (source == BX_RESET_HARDWARE) {

#if BX_SUPPORT_PKEYS
        BX_CPU_THIS_PTR set_PKeys(0, 0);
#endif

#if BX_CPU_LEVEL >= 6
        BX_CPU_THIS_PTR msr.sysenter_cs_msr = 0;
        BX_CPU_THIS_PTR msr.sysenter_esp_msr = 0;
        BX_CPU_THIS_PTR msr.sysenter_eip_msr = 0;
#endif

        // Do not change MTRR on INIT
#if BX_CPU_LEVEL >= 6
        for (n = 0; n < 16; n++)
            BX_CPU_THIS_PTR msr.mtrrphys[n] = 0;

        BX_CPU_THIS_PTR msr.mtrrfix64k = (Bit64u)0; // all fix range MTRRs undefined according to manual
        BX_CPU_THIS_PTR msr.mtrrfix16k[0] = (Bit64u)0;
        BX_CPU_THIS_PTR msr.mtrrfix16k[1] = (Bit64u)0;
        for (n = 0; n < 8; n++)
            BX_CPU_THIS_PTR msr.mtrrfix4k[n] = (Bit64u)0;

        BX_CPU_THIS_PTR msr.pat = (Bit64u)BX_CONST64(0x0007040600070406);
        BX_CPU_THIS_PTR msr.mtrr_deftype = 0;
#endif
        //查错174条
               // All configurable MSRs do not change on INIT
#if BX_CONFIGURE_MSRS
        for (n = 0; n < BX_MSR_MAX_INDEX; n++) {
            if (BX_CPU_THIS_PTR msrs[n])
                BX_CPU_THIS_PTR msrs[n]->reset();
        }
#endif

    }

    BX_CPU_THIS_PTR EXT = 0;
    BX_CPU_THIS_PTR last_exception_type = 0;

    // invalidate the code prefetch queue
    BX_CPU_THIS_PTR eipPageBias = 0;
    BX_CPU_THIS_PTR eipPageWindowSize = 0;
    BX_CPU_THIS_PTR eipFetchPtr = NULL;

    // invalidate current stack page
    BX_CPU_THIS_PTR espPageBias = 0;
    BX_CPU_THIS_PTR espPageWindowSize = 0;
    BX_CPU_THIS_PTR espHostPtr = NULL;
#if BX_SUPPORT_MEMTYPE
    BX_CPU_THIS_PTR espPageMemtype = BX_MEMTYPE_UC;
#endif
#if BX_SUPPORT_SMP == 0
    BX_CPU_THIS_PTR espPageFineGranularityMapping = 0;
#endif

#if BX_DEBUGGER
    BX_CPU_THIS_PTR stop_reason = 0;
    BX_CPU_THIS_PTR magic_break = 0;
    BX_CPU_THIS_PTR trace = 0;
    BX_CPU_THIS_PTR trace_reg = 0;
    BX_CPU_THIS_PTR trace_mem = 0;
    BX_CPU_THIS_PTR mode_break = 0;
#if BX_SUPPORT_VMX
    BX_CPU_THIS_PTR vmexit_break = 0;
#endif
#endif

    // Reset the Floating Point Unit
#if BX_SUPPORT_FPU
    if (source == BX_RESET_HARDWARE) {
        BX_CPU_THIS_PTR the_i387.reset();
    }
#endif

    BX_CPU_THIS_PTR cpu_state_use_ok = 0;

#if BX_CPU_LEVEL >= 6
    // Reset XMM state - unchanged on #INIT
    if (source == BX_RESET_HARDWARE) {
        for (n = 0; n < BX_XMM_REGISTERS; n++) {
            BX_CLEAR_AVX_REG(n);
        }

        BX_CPU_THIS_PTR mxcsr.mxcsr = MXCSR_RESET;
        BX_CPU_THIS_PTR mxcsr_mask = 0x0000ffbf;
        if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_SSE2))
            BX_CPU_THIS_PTR mxcsr_mask |= MXCSR_DAZ;
        if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_MISALIGNED_SSE))
            BX_CPU_THIS_PTR mxcsr_mask |= MXCSR_MISALIGNED_EXCEPTION_MASK;

#if BX_SUPPORT_EVEX
        for (n = 0; n < 8; n++) BX_WRITE_OPMASK(n, 0);
#endif
    }
#endif

#if BX_SUPPORT_VMX
    BX_CPU_THIS_PTR in_vmx = BX_CPU_THIS_PTR in_vmx_guest = false;
    BX_CPU_THIS_PTR in_smm_vmx = BX_CPU_THIS_PTR in_smm_vmx_guest = false;
    BX_CPU_THIS_PTR vmcsptr = BX_CPU_THIS_PTR vmxonptr = BX_INVALID_VMCSPTR;
    set_VMCSPTR(BX_CPU_THIS_PTR vmcsptr);
    if (source == BX_RESET_HARDWARE) {
        BX_CPU_THIS_PTR msr.ia32_feature_ctrl = 0;
    }
#endif

#if BX_SUPPORT_SVM
    set_VMCBPTR(0);
    BX_CPU_THIS_PTR in_svm_guest = false;
    BX_CPU_THIS_PTR svm_gif = true;
#endif

#if BX_SUPPORT_VMX || BX_SUPPORT_SVM
    BX_CPU_THIS_PTR in_event = false;
#endif

#if BX_SUPPORT_VMX
    BX_CPU_THIS_PTR nmi_unblocking_iret = false;
#endif

#if BX_SUPPORT_SMP
    // notice if I'm the bootstrap processor.  If not, do the equivalent of
    // a HALT instruction.
    int apic_id = lapic->get_id();
    if (BX_BOOTSTRAP_PROCESSOR == apic_id) {
        // boot normally
        BX_CPU_THIS_PTR msr.apicbase |= 0x100; /* set bit 8 BSP */
        BX_INFO(("CPU[%d] is the bootstrap processor", apic_id));
    }
    else {
        // it's an application processor, halt until IPI is heard.
        BX_CPU_THIS_PTR msr.apicbase &= ~0x100; /* clear bit 8 BSP */
        BX_INFO(("CPU[%d] is an application processor. Halting until SIPI.", apic_id));
        enter_sleep_state(BX_ACTIVITY_STATE_WAIT_FOR_SIPI);
    }
#endif

    handleCpuContextChange();

#if BX_CPU_LEVEL >= 4
    BX_CPU_THIS_PTR cpuid->dump_cpuid();

    BX_CPU_THIS_PTR cpuid->dump_features();
#endif

    BX_INSTR_RESET(BX_CPU_ID, source);


}



BX_CPU_C::~BX_CPU_C()
{
	//826
#if BX_CPU_LEVEL >= 4
    delete cpuid;
#endif

#if BX_SUPPORT_APIC
    delete lapic;
#endif

#if BX_SUPPORT_AMX
    delete amx;
#endif

#if BX_SUPPORT_SVM
    delete vmcb;
#endif

#if InstrumentCPU
    delete stats;
#endif

#if BX_CPU_LEVEL >= 5
    destroy_MSRs();
#endif

    //BX_INSTR_EXIT(BX_CPU_ID);
    //BX_DEBUG(("Exit."));
}