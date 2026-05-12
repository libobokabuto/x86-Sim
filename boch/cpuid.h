#pragma once
class VMCS_Mapping;

class bx_cpuid_t {
public:
	BX_CPP_INLINE void get_cpu_extensions(Bit32u* extensions) const {
		for (unsigned n = 0; n < BX_ISA_EXTENSIONS_ARRAY_SIZE; n++)
			extensions[n] = ia_extensions_bitmask[n];
	}

	BX_CPP_INLINE bool is_cpu_extension_supported(unsigned extension) const {
		//57
		assert(extension < BX_ISA_EXTENSION_LAST);
		return ia_extensions_bitmask[extension / 32] & (1 << (extension % 32));
	}
#if BX_SUPPORT_VMX
	virtual Bit32u get_vmx_extensions_bitmask(void) const { return 0; }
#endif
#if BX_SUPPORT_SVM
	virtual Bit32u get_svm_extensions_bitmask(void) const { return 0; }
#endif

#if BX_SUPPORT_VMX //82
	VMCS_Mapping* get_vmcs() { return &vmcs_map; }
#endif
	bool support_avx10_512() const; //89

protected:
	//102
	Bit32u ia_extensions_bitmask[BX_ISA_EXTENSIONS_ARRAY_SIZE]; //109
#if BX_SUPPORT_VMX
	VMCS_Mapping vmcs_map;
#endif


#define BX_VMX_VIRTUAL_NMI                      (1 <<  1)
#define BX_VMX_APIC_VIRTUALIZATION              (1 <<  2)
#define BX_VMX_WBINVD_VMEXIT                    (1 <<  3)
#define BX_VMX_X2APIC_VIRTUALIZATION            (1 <<  6)
#define BX_VMX_EPT                              (1 <<  7)  //187
#define BX_VMX_VPID                             (1 <<  8)
#define BX_VMX_UNRESTRICTED_GUEST               (1 <<  9)
#define BX_VMX_PREEMPTION_TIMER                 (1 << 10)
#define BX_VMX_DESCRIPTOR_TABLE_EXIT            (1 << 14)
#define BX_VMX_PAUSE_LOOP_EXITING               (1 << 15)
#define BX_VMX_EPTP_SWITCHING                   (1 << 16) 
#define BX_VMX_EPT_ACCESS_DIRTY                 (1 << 17)
#define BX_VMX_VINTR_DELIVERY                   (1 << 18)
#define BX_VMX_POSTED_INTERRUPTS                (1 << 19)
#define BX_VMX_VMCS_SHADOWING                   (1 << 20)
#define BX_VMX_EPT_EXCEPTION                    (1 << 21)
#define BX_VMX_PML                              (1 << 22)
#define BX_VMX_SPP                              (1 << 23)
#define BX_VMX_TSC_SCALING                      (1 << 24)
#define BX_VMX_MBE_CONTROL                      (1 << 26)
#define BX_VMX_SPEC_CTRL_VIRTUALIZATION         (1 << 27)


#define BX_CPUID_SVM_NESTED_PAGING           (1 <<  0)  //817
#define BX_CPUID_SVM_LBR_VIRTUALIZATION      (1 <<  1)
#define BX_CPUID_SVM_SVM_LOCK                (1 <<  2)
#define BX_CPUID_SVM_NRIP_SAVE               (1 <<  3)
#define BX_CPUID_SVM_TSCRATE                 (1 <<  4)
#define BX_CPUID_SVM_VMCB_CLEAN_BITS         (1 <<  5)
#define BX_CPUID_SVM_FLUSH_BY_ASID           (1 <<  6)
#define BX_CPUID_SVM_DECODE_ASSIST           (1 <<  7)
#define BX_CPUID_SVM_RESERVED8               (1 <<  8)
#define BX_CPUID_SVM_RESERVED9               (1 <<  9)
#define BX_CPUID_SVM_PAUSE_FILTER            (1 << 10)
#define BX_CPUID_SVM_RESERVED11              (1 << 11)
#define BX_CPUID_SVM_PAUSE_FILTER_THRESHOLD  (1 << 12)
#define BX_CPUID_SVM_AVIC                    (1 << 13)
#define BX_CPUID_SVM_RESERVED14              (1 << 14)
#define BX_CPUID_SVM_NESTED_VIRTUALIZATION   (1 << 15)
#define BX_CPUID_SVM_VIRTUAL_GIF             (1 << 16)
#define BX_CPUID_SVM_CMET                    (1 << 17)
};