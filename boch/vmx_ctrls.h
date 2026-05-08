#pragma once
class VmxVmexec1Controls {
private:
	Bit32u vmexec_ctrls;
public:
#define VMX_VM_EXEC_CTRL1_INTERRUPT_WINDOW_VMEXIT   (1 << 2)
#define VMX_VM_EXEC_CTRL1_TSC_OFFSET                (1 << 3)
#define VMX_VM_EXEC_CTRL1_HLT_VMEXIT                (1 << 7)
#define VMX_VM_EXEC_CTRL1_INVLPG_VMEXIT             (1 << 9)
#define VMX_VM_EXEC_CTRL1_MWAIT_VMEXIT              (1 << 10)
#define VMX_VM_EXEC_CTRL1_RDPMC_VMEXIT              (1 << 11)
#define VMX_VM_EXEC_CTRL1_RDTSC_VMEXIT              (1 << 12)
#define VMX_VM_EXEC_CTRL1_DRx_ACCESS_VMEXIT         (1 << 23)
#define VMX_VM_EXEC_CTRL1_IO_VMEXIT                 (1 << 24)
#define VMX_VM_EXEC_CTRL1_IO_BITMAPS                (1 << 25)
#define VMX_VM_EXEC_CTRL1_MSR_BITMAPS               (1 << 28)
#define VMX_VM_EXEC_CTRL1_PAUSE_VMEXIT              (1 << 30)
};
class VmxVmexec2Controls {
private:
	Bit32u vmexec_ctrls;
public:
#define VMX_VM_EXEC_CTRL2_VIRTUALIZE_APIC_ACCESSES  (1 <<  0)
#define VMX_VM_EXEC_CTRL2_EPT_ENABLE                (1 <<  1)
#define VMX_VM_EXEC_CTRL2_DESCRIPTOR_TABLE_VMEXIT   (1 <<  2)
#define VMX_VM_EXEC_CTRL2_RDTSCP                    (1 <<  3)
#define VMX_VM_EXEC_CTRL2_VIRTUALIZE_X2APIC_MODE    (1 <<  4) /* Virtualize X2APIC */
#define VMX_VM_EXEC_CTRL2_VPID_ENABLE               (1 <<  5) /* VPID */
#define VMX_VM_EXEC_CTRL2_WBINVD_VMEXIT             (1 <<  6) /* WBINVD VMEXIT */
#define VMX_VM_EXEC_CTRL2_UNRESTRICTED_GUEST        (1 <<  7) /* Unrestricted Guest */
#define VMX_VM_EXEC_CTRL2_VIRTUALIZE_APIC_REGISTERS (1 <<  8)
#define VMX_VM_EXEC_CTRL2_VIRTUAL_INT_DELIVERY      (1 <<  9)
#define VMX_VM_EXEC_CTRL2_PAUSE_LOOP_VMEXIT         (1 << 10) /* PAUSE loop exiting */
#define VMX_VM_EXEC_CTRL2_RDRAND_VMEXIT             (1 << 11)
#define VMX_VM_EXEC_CTRL2_INVPCID                   (1 << 12)
#define VMX_VM_EXEC_CTRL2_VMFUNC_ENABLE             (1 << 13) /* VM Functions */
#define VMX_VM_EXEC_CTRL2_VMCS_SHADOWING            (1 << 14) /* VMCS Shadowing */
#define VMX_VM_EXEC_CTRL2_SGX_ENCLS_VMEXIT          (1 << 15) /* ENCLS/SGX (not implemented) */
#define VMX_VM_EXEC_CTRL2_RDSEED_VMEXIT             (1 << 16)
#define VMX_VM_EXEC_CTRL2_PML_ENABLE                (1 << 17) /* Page Modification Logging */
#define VMX_VM_EXEC_CTRL2_EPT_VIOLATION_EXCEPTION   (1 << 18)
#define VMX_VM_EXEC_CTRL2_XSAVES_XRSTORS            (1 << 20)
#define VMX_VM_EXEC_CTRL2_MBE_CTRL                  (1 << 22)
#define VMX_VM_EXEC_CTRL2_SUBPAGE_WR_PROTECT_CTRL   (1 << 23)
#define VMX_VM_EXEC_CTRL2_TSC_SCALING               (1 << 25)
#define VMX_VM_EXEC_CTRL2_UMWAIT_TPAUSE_VMEXIT      (1 << 26)

	bool VMFUNC_ENABLE() const { return vmexec_ctrls & VMX_VM_EXEC_CTRL2_VMFUNC_ENABLE; }//137
	bool XSAVES_XRSTORS() const { return vmexec_ctrls & VMX_VM_EXEC_CTRL2_XSAVES_XRSTORS; }
};
class VmxVmexec3Controls {
private:
	Bit64u vmexec_ctrls;
public:

#define VMX_VM_EXEC_CTRL3_ENABLE_MSRLIST            (1 <<  6)
#define VMX_VM_EXEC_CTRL3_VIRTUALIZE_IA32_SPEC_CTRL (1 <<  7)
#define VMX_VM_EXEC_CTRL3_EMULATE_AVX10_VL256       (1 << 13)  //175

	bool VIRTUALIZE_IA32_SPEC_CTRL() const { return vmexec_ctrls & VMX_VM_EXEC_CTRL3_VIRTUALIZE_IA32_SPEC_CTRL; }
	bool EMULATE_AVX10_VL256() const { return vmexec_ctrls & VMX_VM_EXEC_CTRL3_EMULATE_AVX10_VL256; } //184
};

class VmxPinBasedVmexecControls {
private:
	Bit32u pin_vmexec_ctrls;
public:
#define VMX_PIN_BASED_VMEXEC_CTRL_EXTERNAL_INTERRUPT_VMEXIT   (1 << 0)
#define VMX_PIN_BASED_VMEXEC_CTRL_NMI_EXITING                 (1 << 3)
#define VMX_PIN_BASED_VMEXEC_CTRL_VIRTUAL_NMI                 (1 << 5) /* Virtual NMI */
#define VMX_PIN_BASED_VMEXEC_CTRL_VMX_PREEMPTION_TIMER_VMEXIT (1 << 6) /* VMX preemption timer */
#define VMX_PIN_BASED_VMEXEC_CTRL_PROCESS_POSTED_INTERRUPTS   (1 << 7) /* Posted Interrupts */

	bool EXTERNAL_INTERRUPT_VMEXIT() const { return pin_vmexec_ctrls & VMX_PIN_BASED_VMEXEC_CTRL_EXTERNAL_INTERRUPT_VMEXIT; }
};