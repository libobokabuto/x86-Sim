#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"

#define LOG_THIS BX_CPU(0)->
#if BX_SUPPORT_VMX
#include "vmx_ctrls.h"

void BX_CPU_C::init_VMCS(void)
{
	BX_CPU_THIS_PTR vmcs_map = BX_CPU_THIS_PTR cpuid->get_vmcs();

	init_vmx_capabilities();
}

void BX_CPU_C::init_vmx_capabilities(void)
{
#if BX_SUPPORT_VMX >= 2
	init_ept_vpid_capabilities();
	init_vmfunc_capabilities();
#endif
	init_pin_based_vmexec_ctrls();
	init_tertiary_proc_based_vmexec_ctrls(); // must initialize in reverse order
	init_secondary_proc_based_vmexec_ctrls();
	init_primary_proc_based_vmexec_ctrls();
	init_secondary_vmexit_ctrls(); // must initialize in reverse order
	init_vmexit_ctrls();
	init_vmentry_ctrls();

}

#if BX_SUPPORT_VMX >= 2
void BX_CPU_C::init_ept_vpid_capabilities(void)
{
	struct bx_VMX_Cap* cap = &BX_CPU_THIS_PTR vmx_cap;

	// EPT/VPID capabilities
	// -----------------------------------------------------------
	//  [0] - BX_EPT_ENTRY_EXECUTE_ONLY support
	//  [6] - 4-levels EPT page walk length
	//  [8] - allow UC EPT paging structure memory type
	// [14] - allow WB EPT paging structure memory type
	// [16] - EPT 2M pages support
	// [17] - EPT 1G pages support
	// [20] - INVEPT instruction supported
	// [21] - EPT A/D bits supported
	// [22] - advanced VM-exit information for EPT violations (tied to MBE support)
	// [23] - Enable Shadow Stack control bit is supported in EPTP (CET)
	// [25] - INVEPT single-context invalidation supported
	// [26] - INVEPT all-context invalidation supported
	// [32] - INVVPID instruction supported
	// [40] - individual-address INVVPID is supported
	// [41] - single-context INVVPID is supported
	// [42] - all-context INVVPID is supported
	// [43] - single-context-retaining-globals INVVPID is supported

	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT)) {
		cap->vmx_ept_vpid_cap_supported_bits = BX_CONST64(0x06114141);
		if (is_cpu_extension_supported(BX_ISA_1G_PAGES))
			cap->vmx_ept_vpid_cap_supported_bits |= (1 << 17);
		if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT_ACCESS_DIRTY))
			cap->vmx_ept_vpid_cap_supported_bits |= (1 << 21);
		if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_MBE_CONTROL))
			cap->vmx_ept_vpid_cap_supported_bits |= (1 << 22);
		if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_CET))
			cap->vmx_ept_vpid_cap_supported_bits |= (1 << 23);
	}
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_VPID))
		cap->vmx_ept_vpid_cap_supported_bits |= BX_CONST64(0x00000f01) << 32;
}

void BX_CPU_C::init_vmfunc_capabilities(void)
{
	struct bx_VMX_Cap* cap = &BX_CPU_THIS_PTR vmx_cap;

	// vm functions
	// -----------------------------------------------------------
	//    [00] EPTP switching
	// [63-01] reserved

	cap->vmx_vmfunc_supported_bits = 0;

	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPTP_SWITCHING))
		cap->vmx_vmfunc_supported_bits |= VMX_VMFUNC_EPTP_SWITCHING_MASK;
}
#endif

void BX_CPU_C::init_pin_based_vmexec_ctrls(void)
{
	//647
	struct bx_VMX_Cap* cap = &BX_CPU_THIS_PTR vmx_cap;
	// pin based vm exec controls
  // -----------------------------------------------------------
  //   [00] External Interrupt Exiting
  // 1 [01] Reserved (must be '1)
  // 1 [02] Reserved (must be '1)
  //   [03] NMI Exiting
  // 1 [04] Reserved (must be '1)
  //   [05] Virtual NMI (require Virtual NMI support)
  //   [06] Activate VMX Preemption Timer (require VMX Preemption Timer support)
  //   [07] Process Posted interrupts

	cap->vmx_pin_vmexec_ctrl_supported_bits =
		VMX_PIN_BASED_VMEXEC_CTRL_EXTERNAL_INTERRUPT_VMEXIT |
		VMX_PIN_BASED_VMEXEC_CTRL_NMI_EXITING;
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_VIRTUAL_NMI))
		cap->vmx_pin_vmexec_ctrl_supported_bits |= VMX_PIN_BASED_VMEXEC_CTRL_VIRTUAL_NMI;
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_PREEMPTION_TIMER))
		cap->vmx_pin_vmexec_ctrl_supported_bits |= VMX_PIN_BASED_VMEXEC_CTRL_VMX_PREEMPTION_TIMER_VMEXIT;
#endif
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_POSTED_INTERRUPTS))
		cap->vmx_pin_vmexec_ctrl_supported_bits |= VMX_PIN_BASED_VMEXEC_CTRL_PROCESS_POSTED_INTERRUPTS;
}

void BX_CPU_C::init_primary_proc_based_vmexec_ctrls(void)
{

}

void BX_CPU_C::init_secondary_proc_based_vmexec_ctrls(void)
{
	//758
	struct bx_VMX_Cap* cap = &BX_CPU_THIS_PTR vmx_cap;

	// secondary proc based vm exec controls
	// -----------------------------------------------------------
	//   [00] Apic Virtualization (require x86-64 for TPR shadow)
	//   [01] EPT Enable (require x86-64 for 4-level page walk)
	//   [02] Descriptor Table Exiting
	//   [03] Enable RDTSCP instruction (RDTSCP will #UD if not set)
	//   [04] Virtualize X2APIC Mode (doesn't require actual X2APIC to be enabled)
	//   [05] VPID Enable
	//   [06] WBINVD Exiting
	//   [07] Unrestricted Guest (require EPT)
	//   [08] Virtualize Apic Registers
	//   [09] Virtualize Interrupt Delivery
	//   [10] PAUSE Loop Exiting
	//   [11] RDRAND Exiting (require RDRAND instruction support)
	//   [12] Enable INVPCID instruction (require INVPCID instruction support)
	//   [13] Enable VM Functions
	//   [14] Enable VMCS Shadowing
	//   [15] Reserved (must be '0)
	//   [16] RDSEED Exiting (require RDSEED instruction support)
	//   [17] Page Modification Logging Enable
	//   [18] Support for EPT Violation (#VE) exception
	//   [19] Reserved (must be '0)
	//   [20] XSAVES Exiting
	//   [21] Reserved (must be '0)
	//   [22] Mode Based Execution Control (MBE)
	//   [23] Sub Page Protection
	//   [24] Processor Trace use GPA (not implemented)
	//   [25] Enable TSC Scaling
	//   [26] UMWAIT/TPAUSE Exiting

	cap->vmx_vmexec_ctrl2_supported_bits = 0;

#if BX_SUPPORT_X86_64
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_APIC_VIRTUALIZATION))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VIRTUALIZE_APIC_ACCESSES;
#endif
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_EPT_ENABLE;
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_DESCRIPTOR_TABLE_EXIT))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_DESCRIPTOR_TABLE_VMEXIT;
#endif
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_RDTSCP))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_RDTSCP;
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_X2APIC_VIRTUALIZATION))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VIRTUALIZE_X2APIC_MODE;
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_VPID))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VPID_ENABLE;
#endif
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_WBINVD_VMEXIT))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_WBINVD_VMEXIT;
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_UNRESTRICTED_GUEST))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_UNRESTRICTED_GUEST;
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_VINTR_DELIVERY))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VIRTUALIZE_APIC_REGISTERS | VMX_VM_EXEC_CTRL2_VIRTUAL_INT_DELIVERY;
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_PAUSE_LOOP_EXITING))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_PAUSE_LOOP_VMEXIT;
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_INVPCID))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_INVPCID;
#endif
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_RDRAND))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_RDRAND_VMEXIT;
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_VMCS_SHADOWING))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VMCS_SHADOWING;
#endif
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_RDSEED))
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_RDSEED_VMEXIT;
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_PML)) {
		if (!BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT))
			//BX_PANIC(("VMX PML feature requires EPT support !"));
			cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_PML_ENABLE;
	}
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT_EXCEPTION)) {
		if (!BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPTP_SWITCHING))
			//BX_PANIC(("#VE exception feature requires EPTP switching support !"));
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_EPT_VIOLATION_EXCEPTION;
	}
#endif
#if BX_SUPPORT_VMX >= 2
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_XSAVES)) {
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_XSAVES_XRSTORS;
	}
#endif
#if BX_SUPPORT_VMX >= 2
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_MBE_CONTROL)) {
		if (!BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT))
			//BX_PANIC(("VMX MBE feature requires EPT support !"));
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_MBE_CTRL;
	}
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_SPP)) {
		if (!BX_SUPPORT_VMX_EXTENSION(BX_VMX_EPT))
			//BX_PANIC(("VMX SPP feature requires EPT support !"));
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_SUBPAGE_WR_PROTECT_CTRL;
	}
#endif
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_TSC_SCALING)) {
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_TSC_SCALING;
	}
#if BX_SUPPORT_MONITOR_MWAIT 
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_WAITPKG)) {
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_UMWAIT_TPAUSE_VMEXIT;
	}
#endif

#if BX_SUPPORT_VMX >= 2
	// enable vm functions secondary vmexec control if there are supported vmfunctions
	if (cap->vmx_vmfunc_supported_bits != 0)
		cap->vmx_vmexec_ctrl2_supported_bits |= VMX_VM_EXEC_CTRL2_VMFUNC_ENABLE;
#endif
}

void BX_CPU_C::init_tertiary_proc_based_vmexec_ctrls(void)
{
	//877
	struct bx_VMX_Cap* cap = &BX_CPU_THIS_PTR vmx_cap;
	// tertiary proc based vm exec controls
  // -----------------------------------------------------------
  //   [00] LOADIWKEY exiting (KeyLocker)
  //   [01] Enable HLAT
  //   [02] EPT Paging Write control
  //   [03] Guest Paging verification
  //   [04] IPI Virtualization
  //    ...
  //   [06] Enable MSRLIST instructions
  //   [07] Virtualize IA32_SPEC_CTRL
  //    ...
  //   [13] Emulate AVX10.VL256 mode
  //    ...

	cap->vmx_vmexec_ctrl3_supported_bits = 0;

	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_MSRLIST)) {
		cap->vmx_vmexec_ctrl3_supported_bits |= VMX_VM_EXEC_CTRL3_ENABLE_MSRLIST;
	}
	if (BX_SUPPORT_VMX_EXTENSION(BX_VMX_SPEC_CTRL_VIRTUALIZATION)) {
		cap->vmx_vmexec_ctrl3_supported_bits |= VMX_VM_EXEC_CTRL3_VIRTUALIZE_IA32_SPEC_CTRL;
	}
	if (BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX10_1) && BX_CPUID_SUPPORT_ISA_EXTENSION(BX_ISA_AVX10_VL512)) {
		cap->vmx_vmexec_ctrl3_supported_bits |= VMX_VM_EXEC_CTRL3_EMULATE_AVX10_VL256;
	}

}

void BX_CPU_C::init_secondary_vmexit_ctrls(void)
{

}

void BX_CPU_C::init_vmexit_ctrls(void)
{

}

void BX_CPU_C::init_vmentry_ctrls(void)
{

}

#endif