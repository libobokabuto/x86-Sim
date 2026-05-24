#pragma once
class VMCS_Mapping;

struct cpuid_function_t {
	Bit32u eax;
	Bit32u ebx;
	Bit32u ecx;
	Bit32u edx;

	bool is_empty() { return (eax | ebx | ecx | edx) == 0; }
};
class VMCS_Mapping;

class bx_cpuid_t {
public:
	bx_cpuid_t(BX_CPU_C* _cpu);
#if BX_SUPPORT_VMX
	bx_cpuid_t(BX_CPU_C* _cpu, Bit32u vmcs_revision);
	bx_cpuid_t(BX_CPU_C* _cpu, Bit32u vmcs_revision, const char* filename);
#endif
	virtual ~bx_cpuid_t() {}

	void init();

	virtual const char* get_name(void) const = 0;

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

	virtual void get_cpuid_leaf(Bit32u function, Bit32u subfunction, cpuid_function_t* leaf) const = 0;

	virtual void dump_cpuid(void) const = 0;

	void sanity_checks() const;

#if BX_CPU_LEVEL >= 5
	virtual int rdmsr(Bit32u index, Bit64u* msr) { return -1; }
	virtual int wrmsr(Bit32u index, Bit64u  msr) { return -1; }
#endif

#if BX_SUPPORT_VMX //82
	VMCS_Mapping* get_vmcs() { return &vmcs_map; }
#endif
	bool support_avx10_512() const; //89

	BX_CPP_INLINE void enable_cpu_extension(unsigned extension) {
		assert(extension < BX_ISA_EXTENSION_LAST);
		ia_extensions_bitmask[extension / 32] |= (1 << (extension % 32));
		warning_messages(extension);
	}

	BX_CPP_INLINE void disable_cpu_extension(unsigned extension) {
		assert(extension < BX_ISA_EXTENSION_LAST);
		ia_extensions_bitmask[extension / 32] &= ~(1 << (extension % 32));
	}

protected:
	BX_CPU_C* cpu;

	unsigned nprocessors;
	unsigned ncores;
	unsigned nthreads;
	//102
	Bit32u ia_extensions_bitmask[BX_ISA_EXTENSIONS_ARRAY_SIZE]; //109

	void get_leaf_0(unsigned max_leaf, const char* vendor_string, cpuid_function_t* leaf, unsigned limited_max_leaf = 0x02) const;
	void get_ext_cpuid_brand_string_leaf(const char* brand_string, Bit32u function, cpuid_function_t* leaf) const;
	
#if BX_SUPPORT_APIC
	void get_std_cpuid_extended_topology_leaf(Bit32u subfunction, cpuid_function_t* leaf) const;
#endif

#if BX_CPU_LEVEL >= 6
	void get_std_cpuid_xsave_leaf(Bit32u subfunction, cpuid_function_t* leaf) const;
#endif

	void get_std_cpuid_monitor_mwait_leaf(cpuid_function_t* leaf, Bit32u edx_power_states) const;

	Bit32u get_std_cpuid_leaf_1_ecx(Bit32u extra = 0) const;
	Bit32u get_std_cpuid_leaf_1_edx_common(Bit32u extra = 0) const;
	Bit32u get_std_cpuid_leaf_1_edx(Bit32u extra = 0) const;
	Bit32u get_std_cpuid_leaf_7_ebx(Bit32u extra = 0) const;

	Bit32u get_ext_cpuid_leaf_1_ecx(Bit32u extra = 0) const;
	Bit32u get_ext_cpuid_leaf_1_edx_intel() const;

	void get_ext_cpuid_leaf_8(cpuid_function_t* leaf) const;

	BX_CPP_INLINE void get_leaf(cpuid_function_t* leaf, Bit32u eax, Bit32u ebx, Bit32u ecx, Bit32u edx) const
	{
		leaf->eax = eax;
		leaf->ebx = ebx;
		leaf->ecx = ecx;
		leaf->edx = edx;
	}

	BX_CPP_INLINE void get_reserved_leaf(cpuid_function_t* leaf) const
	{
		leaf->eax = 0;
		leaf->ebx = 0;
		leaf->ecx = 0;
		leaf->edx = 0;
	}

	void dump_cpuid_leaf(unsigned function, unsigned subfunction = 0) const;
	void dump_cpuid(unsigned max_std_leaf, unsigned max_ext_leaf) const;

	void warning_messages(unsigned extension) const;

#if BX_SUPPORT_VMX
	VMCS_Mapping vmcs_map;
#endif

};

#define BX_VMX_TPR_SHADOW                       (1 <<  0)   /* TPR shadow */
#define BX_VMX_VIRTUAL_NMI                      (1 <<  1)   /* Virtual NMI */
#define BX_VMX_APIC_VIRTUALIZATION              (1 <<  2)   /* APIC Access Virtualization */
#define BX_VMX_WBINVD_VMEXIT                    (1 <<  3)   /* WBINVD VMEXIT */
#define BX_VMX_PERF_GLOBAL_CTRL                 (1 <<  4)   /* Save/Restore MSR_PERF_GLOBAL_CTRL */
#define BX_VMX_MONITOR_TRAP_FLAG                (1 <<  5)   /* Monitor trap Flag (MTF) */
#define BX_VMX_X2APIC_VIRTUALIZATION            (1 <<  6)   /* Virtualize X2APIC */
#define BX_VMX_EPT                              (1 <<  7)   /* Extended Page Tables (EPT) */
#define BX_VMX_VPID                             (1 <<  8)   /* VPID */
#define BX_VMX_UNRESTRICTED_GUEST               (1 <<  9)   /* Unrestricted Guest */
#define BX_VMX_PREEMPTION_TIMER                 (1 << 10)   /* VMX preemption timer */
#define BX_VMX_SAVE_DEBUGCTL_DISABLE            (1 << 11)   /* Disable Save/Restore of MSR_DEBUGCTL */
#define BX_VMX_PAT                              (1 << 12)   /* Save/Restore MSR_PAT */
#define BX_VMX_EFER                             (1 << 13)   /* Save/Restore MSR_EFER */
#define BX_VMX_DESCRIPTOR_TABLE_EXIT            (1 << 14)   /* Descriptor Table VMEXIT */
#define BX_VMX_PAUSE_LOOP_EXITING               (1 << 15)   /* Pause Loop Exiting */
#define BX_VMX_EPTP_SWITCHING                   (1 << 16)   /* EPTP switching (VM Function 0) */
#define BX_VMX_EPT_ACCESS_DIRTY                 (1 << 17)   /* Extended Page Tables (EPT) A/D Bits */
#define BX_VMX_VINTR_DELIVERY                   (1 << 18)   /* Virtual Interrupt Delivery */
#define BX_VMX_POSTED_INTERRUPTS                (1 << 19)   /* Posted Interrupts support */
#define BX_VMX_VMCS_SHADOWING                   (1 << 20)   /* VMCS Shadowing */
#define BX_VMX_EPT_EXCEPTION                    (1 << 21)   /* EPT Violation (#VE) exception */
#define BX_VMX_PML                              (1 << 22)   /* Page Modification Logging */
#define BX_VMX_SPP                              (1 << 23)   /* Sub Page Protection */
#define BX_VMX_TSC_SCALING                      (1 << 24)   /* TSC Scaling */
#define BX_VMX_SW_INTERRUPT_INJECTION_ILEN_0    (1 << 25)   /* Allow software interrupt injection with instruction length 0 */
#define BX_VMX_MBE_CONTROL                      (1 << 26)   /* Mode-Based Execution Control (XU/XS) */
#define BX_VMX_SPEC_CTRL_VIRTUALIZATION         (1 << 27)

#define BX_CPUID_STD1_EDX_X87                     (1 <<  0)
#define BX_CPUID_STD1_EDX_VME                     (1 <<  1)
#define BX_CPUID_STD1_EDX_DEBUG_EXTENSIONS        (1 <<  2)
#define BX_CPUID_STD1_EDX_PSE                     (1 <<  3)
#define BX_CPUID_STD1_EDX_TSC                     (1 <<  4)
#define BX_CPUID_STD1_EDX_MSR                     (1 <<  5)
#define BX_CPUID_STD1_EDX_PAE                     (1 <<  6)
#define BX_CPUID_STD1_EDX_MCE                     (1 <<  7)
#define BX_CPUID_STD1_EDX_CMPXCHG8B               (1 <<  8)
#define BX_CPUID_STD1_EDX_APIC                    (1 <<  9)
#define BX_CPUID_STD1_EDX_RESERVED10              (1 << 10)
#define BX_CPUID_STD1_EDX_SYSENTER_SYSEXIT        (1 << 11)
#define BX_CPUID_STD1_EDX_MTRR                    (1 << 12)
#define BX_CPUID_STD1_EDX_GLOBAL_PAGES            (1 << 13)
#define BX_CPUID_STD1_EDX_MCA                     (1 << 14)
#define BX_CPUID_STD1_EDX_CMOV                    (1 << 15)
#define BX_CPUID_STD1_EDX_PAT                     (1 << 16)
#define BX_CPUID_STD1_EDX_PSE36                   (1 << 17)
#define BX_CPUID_STD1_EDX_PROCESSOR_SERIAL_NUMBER (1 << 18)
#define BX_CPUID_STD1_EDX_CLFLUSH                 (1 << 19)
#define BX_CPUID_STD1_EDX_RESERVED20              (1 << 20)
#define BX_CPUID_STD1_EDX_DEBUG_STORE             (1 << 21)
#define BX_CPUID_STD1_EDX_ACPI                    (1 << 22)
#define BX_CPUID_STD1_EDX_MMX                     (1 << 23)
#define BX_CPUID_STD1_EDX_FXSAVE_FXRSTOR          (1 << 24)
#define BX_CPUID_STD1_EDX_SSE                     (1 << 25)
#define BX_CPUID_STD1_EDX_SSE2                    (1 << 26)
#define BX_CPUID_STD1_EDX_SELF_SNOOP              (1 << 27)
#define BX_CPUID_STD1_EDX_HT                      (1 << 28)
#define BX_CPUID_STD1_EDX_THERMAL_MONITOR         (1 << 29)
#define BX_CPUID_STD1_EDX_RESERVED30              (1 << 30)
#define BX_CPUID_STD1_EDX_PBE                     (1 << 31)

#define BX_CPUID_STD1_ECX_SSE3                    (1 <<  0)
#define BX_CPUID_STD1_ECX_PCLMULQDQ               (1 <<  1)
#define BX_CPUID_STD1_ECX_DTES64                  (1 <<  2)
#define BX_CPUID_STD1_ECX_MONITOR_MWAIT           (1 <<  3)
#define BX_CPUID_STD1_ECX_DS_CPL                  (1 <<  4)
#define BX_CPUID_STD1_ECX_VMX                     (1 <<  5)
#define BX_CPUID_STD1_ECX_SMX                     (1 <<  6)
#define BX_CPUID_STD1_ECX_EST                     (1 <<  7)
#define BX_CPUID_STD1_ECX_THERMAL_MONITOR2        (1 <<  8)
#define BX_CPUID_STD1_ECX_SSSE3                   (1 <<  9)
#define BX_CPUID_STD1_ECX_CNXT_ID                 (1 << 10)
#define BX_CPUID_STD1_ECX_DEBUG_INTERFACE         (1 << 11)
#define BX_CPUID_STD1_ECX_FMA                     (1 << 12)
#define BX_CPUID_STD1_ECX_CMPXCHG16B              (1 << 13)
#define BX_CPUID_STD1_ECX_xTPR                    (1 << 14)
#define BX_CPUID_STD1_ECX_PDCM                    (1 << 15)
#define BX_CPUID_STD1_ECX_RESERVED16              (1 << 16)
#define BX_CPUID_STD1_ECX_PCID                    (1 << 17)
#define BX_CPUID_STD1_ECX_DCA                     (1 << 18)
#define BX_CPUID_STD1_ECX_SSE4_1                  (1 << 19)
#define BX_CPUID_STD1_ECX_SSE4_2                  (1 << 20)
#define BX_CPUID_STD1_ECX_X2APIC                  (1 << 21)
#define BX_CPUID_STD1_ECX_MOVBE                   (1 << 22)
#define BX_CPUID_STD1_ECX_POPCNT                  (1 << 23)
#define BX_CPUID_STD1_ECX_TSC_DEADLINE            (1 << 24)
#define BX_CPUID_STD1_ECX_AES                     (1 << 25)
#define BX_CPUID_STD1_ECX_XSAVE                   (1 << 26)
#define BX_CPUID_STD1_ECX_OSXSAVE                 (1 << 27)
#define BX_CPUID_STD1_ECX_AVX                     (1 << 28)
#define BX_CPUID_STD1_ECX_AVX_F16C                (1 << 29)
#define BX_CPUID_STD1_ECX_RDRAND                  (1 << 30)
#define BX_CPUID_STD1_ECX_RESERVED31              (1 << 31)

#define BX_CPUID_STD7_SUBLEAF0_EBX_FSGSBASE               (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF0_EBX_TSC_ADJUST             (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF0_EBX_SGX                    (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF0_EBX_BMI1                   (1 <<  3)
#define BX_CPUID_STD7_SUBLEAF0_EBX_HLE                    (1 <<  4)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX2                   (1 <<  5)
#define BX_CPUID_STD7_SUBLEAF0_EBX_FDP_DEPRECATION        (1 <<  6)
#define BX_CPUID_STD7_SUBLEAF0_EBX_SMEP                   (1 <<  7)
#define BX_CPUID_STD7_SUBLEAF0_EBX_BMI2                   (1 <<  8)
#define BX_CPUID_STD7_SUBLEAF0_EBX_ENCHANCED_REP_STRINGS  (1 <<  9)
#define BX_CPUID_STD7_SUBLEAF0_EBX_INVPCID                (1 << 10)
#define BX_CPUID_STD7_SUBLEAF0_EBX_RTM                    (1 << 11)
#define BX_CPUID_STD7_SUBLEAF0_EBX_QOS_MONITORING         (1 << 12)
#define BX_CPUID_STD7_SUBLEAF0_EBX_DEPRECATE_FCS_FDS      (1 << 13)
#define BX_CPUID_STD7_SUBLEAF0_EBX_MPX                    (1 << 14)
#define BX_CPUID_STD7_SUBLEAF0_EBX_QOS_ENFORCEMENT        (1 << 15)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512F                (1 << 16)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512DQ               (1 << 17)
#define BX_CPUID_STD7_SUBLEAF0_EBX_RDSEED                 (1 << 18)
#define BX_CPUID_STD7_SUBLEAF0_EBX_ADX                    (1 << 19)
#define BX_CPUID_STD7_SUBLEAF0_EBX_SMAP                   (1 << 20)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512IFMA52           (1 << 21)
#define BX_CPUID_STD7_SUBLEAF0_EBX_RESERVED22             (1 << 22)
#define BX_CPUID_STD7_SUBLEAF0_EBX_CLFLUSHOPT             (1 << 23)
#define BX_CPUID_STD7_SUBLEAF0_EBX_CLWB                   (1 << 24)
#define BX_CPUID_STD7_SUBLEAF0_EBX_PROCESSOR_TRACE        (1 << 25)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512PF               (1 << 26)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512ER               (1 << 27)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512CD               (1 << 28)
#define BX_CPUID_STD7_SUBLEAF0_EBX_SHA                    (1 << 29)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512BW               (1 << 30)
#define BX_CPUID_STD7_SUBLEAF0_EBX_AVX512VL               (1 << 31)

#define BX_CPUID_STD7_SUBLEAF0_ECX_PREFETCHWT1            (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF0_ECX_AVX512_VBMI            (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF0_ECX_UMIP                   (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF0_ECX_PKU                    (1 <<  3)
#define BX_CPUID_STD7_SUBLEAF0_ECX_OSPKE                  (1 <<  4)
#define BX_CPUID_STD7_SUBLEAF0_ECX_WAITPKG                (1 <<  5)
#define BX_CPUID_STD7_SUBLEAF0_ECX_AVX512_VBMI2           (1 <<  6)
#define BX_CPUID_STD7_SUBLEAF0_ECX_CET_SS                 (1 <<  7)
#define BX_CPUID_STD7_SUBLEAF0_ECX_GFNI                   (1 <<  8)
#define BX_CPUID_STD7_SUBLEAF0_ECX_VAES                   (1 <<  9)
#define BX_CPUID_STD7_SUBLEAF0_ECX_VPCLMULQDQ             (1 << 10)
#define BX_CPUID_STD7_SUBLEAF0_ECX_AVX512_VNNI            (1 << 11)
#define BX_CPUID_STD7_SUBLEAF0_ECX_AVX512_BITALG          (1 << 12)
#define BX_CPUID_STD7_SUBLEAF0_ECX_TME                    (1 << 13)
#define BX_CPUID_STD7_SUBLEAF0_ECX_AVX512_VPOPCNTDQ       (1 << 14)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED15             (1 << 15)
#define BX_CPUID_STD7_SUBLEAF0_ECX_LA57                   (1 << 16)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED17             (1 << 17)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED18             (1 << 18)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED19             (1 << 19)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED20             (1 << 20)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED21             (1 << 21)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RDPID                  (1 << 22)
#define BX_CPUID_STD7_SUBLEAF0_ECX_KEYLOCKER              (1 << 23)
#define BX_CPUID_STD7_SUBLEAF0_ECX_BUS_LOCK_DETECT        (1 << 24)
#define BX_CPUID_STD7_SUBLEAF0_ECX_CLDEMOTE               (1 << 25)
#define BX_CPUID_STD7_SUBLEAF0_ECX_RESERVED26             (1 << 26)
#define BX_CPUID_STD7_SUBLEAF0_ECX_MOVDIRI                (1 << 27)
#define BX_CPUID_STD7_SUBLEAF0_ECX_MOVDIR64B              (1 << 28)
#define BX_CPUID_STD7_SUBLEAF0_ECX_ENQCMD                 (1 << 29)
#define BX_CPUID_STD7_SUBLEAF0_ECX_SGX_LAUNCH_CONFIG      (1 << 30)
#define BX_CPUID_STD7_SUBLEAF0_ECX_PKS                    (1 << 31)

#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED0              (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SGX_KEYS               (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AVX512_4VNNIW          (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AVX512_4FMAPS          (1 <<  3)
#define BX_CPUID_STD7_SUBLEAF0_EDX_FAST_SHORT_REP_MOV     (1 <<  4)
#define BX_CPUID_STD7_SUBLEAF0_EDX_UINTR                  (1 <<  5)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED6              (1 <<  6)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED7              (1 <<  7)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AVX512_VPINTERSECT     (1 <<  8)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SRBDS_CTRL             (1 <<  9)
#define BX_CPUID_STD7_SUBLEAF0_EDX_MD_CLEAR               (1 << 10)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RTM_ALWAYS_ABORT       (1 << 11)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED12             (1 << 12)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RTM_FORCE_ABORT        (1 << 13)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SERIALIZE              (1 << 14)
#define BX_CPUID_STD7_SUBLEAF0_EDX_HYBRID                 (1 << 15)
#define BX_CPUID_STD7_SUBLEAF0_EDX_TSXLDTRK               (1 << 16)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED17             (1 << 17)
#define BX_CPUID_STD7_SUBLEAF0_EDX_PCONFIG                (1 << 18)
#define BX_CPUID_STD7_SUBLEAF0_EDX_ARCH_LBR               (1 << 19)
#define BX_CPUID_STD7_SUBLEAF0_EDX_CET_IBT                (1 << 20)
#define BX_CPUID_STD7_SUBLEAF0_EDX_RESERVED21             (1 << 21)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AMX_BF16               (1 << 22)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AVX512_FP16            (1 << 23)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AMX_TILE               (1 << 24)
#define BX_CPUID_STD7_SUBLEAF0_EDX_AMX_INT8               (1 << 25)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SCA_IBRS_IBPB          (1 << 26)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SCA_STIBP              (1 << 27)
#define BX_CPUID_STD7_SUBLEAF0_EDX_L1D_FLUSH              (1 << 28)
#define BX_CPUID_STD7_SUBLEAF0_EDX_ARCH_CAPABILITIES_MSR  (1 << 29)
#define BX_CPUID_STD7_SUBLEAF0_EDX_CORE_CAPABILITIES_MSR  (1 << 30)
#define BX_CPUID_STD7_SUBLEAF0_EDX_SCA_SSBD               (1 << 31)

#define BX_CPUID_STD7_SUBLEAF1_EAX_SHA512                 (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF1_EAX_SM3                    (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF1_EAX_SM4                    (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RAO_INT                (1 <<  3)
#define BX_CPUID_STD7_SUBLEAF1_EAX_AVX_VNNI               (1 <<  4)
#define BX_CPUID_STD7_SUBLEAF1_EAX_AVX512_BF16            (1 <<  5)
#define BX_CPUID_STD7_SUBLEAF1_EAX_LASS                   (1 <<  6)
#define BX_CPUID_STD7_SUBLEAF1_EAX_CMPCCXADD              (1 <<  7)
#define BX_CPUID_STD7_SUBLEAF1_EAX_ARCH_PERFMON           (1 <<  8)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED9              (1 <<  9)
#define BX_CPUID_STD7_SUBLEAF1_EAX_FAST_ZEROLEN_REP_MOVSB (1 << 10)
#define BX_CPUID_STD7_SUBLEAF1_EAX_FAST_ZEROLEN_REP_STOSB (1 << 11)
#define BX_CPUID_STD7_SUBLEAF1_EAX_FAST_ZEROLEN_REP_CMPSB (1 << 12)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED13             (1 << 13)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED14             (1 << 14)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED15             (1 << 15)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED16             (1 << 16)
#define BX_CPUID_STD7_SUBLEAF1_EAX_FRED                   (1 << 17)
#define BX_CPUID_STD7_SUBLEAF1_EAX_LKGS                   (1 << 18)
#define BX_CPUID_STD7_SUBLEAF1_EAX_WRMSRNS                (1 << 19)
#define BX_CPUID_STD7_SUBLEAF1_EAX_NMI_SOURCE_REPORTING   (1 << 20)
#define BX_CPUID_STD7_SUBLEAF1_EAX_AMX_FP16               (1 << 21)
#define BX_CPUID_STD7_SUBLEAF1_EAX_HRESET                 (1 << 22)
#define BX_CPUID_STD7_SUBLEAF1_EAX_AVX_IFMA               (1 << 23)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED24             (1 << 24)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED25             (1 << 25)
#define BX_CPUID_STD7_SUBLEAF1_EAX_LAM                    (1 << 26)
#define BX_CPUID_STD7_SUBLEAF1_EAX_MSRLIST                (1 << 27)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED28             (1 << 28)
#define BX_CPUID_STD7_SUBLEAF1_EAX_RESERVED29             (1 << 29)
#define BX_CPUID_STD7_SUBLEAF1_EAX_INVD_DISABLE           (1 << 30)
#define BX_CPUID_STD7_SUBLEAF1_EAX_MOVRS                  (1 << 31)

#define BX_CPUID_STD7_SUBLEAF1_EBX_PPIN                   (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF1_EBX_TSE                    (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF1_EBX_RESERVED2              (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF1_EBX_CPUIDMAXVAL_LIM_RMV    (1 <<  3)

#define BX_CPUID_STD7_SUBLEAF1_ECX_MSR_IMM                (1 <<  5)

#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED0              (1 <<  0)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED1              (1 <<  1)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED2              (1 <<  2)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED3              (1 <<  3)
#define BX_CPUID_STD7_SUBLEAF1_EDX_AVX_VNNI_INT8          (1 <<  4)
#define BX_CPUID_STD7_SUBLEAF1_EDX_AVX_NE_CONVERT         (1 <<  5)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED6              (1 <<  6)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED7              (1 <<  7)
#define BX_CPUID_STD7_SUBLEAF1_EDX_AMX_COMPLEX            (1 <<  8)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED9              (1 <<  9)
#define BX_CPUID_STD7_SUBLEAF1_EDX_AVX_VNNI_INT16         (1 << 10)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED11             (1 << 11)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED12             (1 << 12)
#define BX_CPUID_STD7_SUBLEAF1_EDX_USER_TIMER             (1 << 13)
#define BX_CPUID_STD7_SUBLEAF1_EDX_PREFETCHI              (1 << 14)
#define BX_CPUID_STD7_SUBLEAF1_EDX_USER_MSR               (1 << 15)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED16             (1 << 16)
#define BX_CPUID_STD7_SUBLEAF1_EDX_UIRET_UIF              (1 << 17)
#define BX_CPUID_STD7_SUBLEAF1_EDX_CET_SSS                (1 << 18)
#define BX_CPUID_STD7_SUBLEAF1_EDX_AVX10                  (1 << 19)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED20             (1 << 20)
#define BX_CPUID_STD7_SUBLEAF1_EDX_APX                    (1 << 21)
#define BX_CPUID_STD7_SUBLEAF1_EDX_RESERVED22             (1 << 22)
#define BX_CPUID_STD7_SUBLEAF1_EDX_MWAIT_AND_LEAF5        (1 << 23)

#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_INT8              (1 <<  0)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_BF16              (1 <<  1)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_COMPLEX           (1 <<  2)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_FP16              (1 <<  3)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_FP8               (1 <<  4)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_TRANSPOSE         (1 <<  5)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_TF32              (1 <<  6)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_AVX512            (1 <<  7)
#define BX_CPUID_AMX_EXTENSIONS_EAX_AMX_MOVRS             (1 <<  8)

#define BX_CPUID_EXT1_EDX_SYSCALL_SYSRET         (1 << 11)

#define BX_CPUID_EXT1_EDX_NX                     (1 << 20)
#define BX_CPUID_EXT1_EDX_RESERVED21             (1 << 21)
#define BX_CPUID_EXT1_EDX_AMD_MMX_EXT            (1 << 22)
#define BX_CPUID_EXT1_EDX_RESERVED23             (1 << 23)
#define BX_CPUID_EXT1_EDX_RESERVED24             (1 << 24)
#define BX_CPUID_EXT1_EDX_FFXSR                  (1 << 25)
#define BX_CPUID_EXT1_EDX_1G_PAGES               (1 << 26)
#define BX_CPUID_EXT1_EDX_RDTSCP                 (1 << 27)
#define BX_CPUID_EXT1_EDX_RESERVED28             (1 << 28)
#define BX_CPUID_EXT1_EDX_LONG_MODE              (1 << 29)
#define BX_CPUID_EXT1_EDX_3DNOW_EXT              (1 << 30)
#define BX_CPUID_EXT1_EDX_3DNOW                  (1 << 31)

#define BX_CPUID_EXT1_ECX_LAHF_SAHF              (1 <<  0)
#define BX_CPUID_EXT1_ECX_CMP_LEGACY             (1 <<  1)
#define BX_CPUID_EXT1_ECX_SVM                    (1 <<  2)
#define BX_CPUID_EXT1_ECX_EXT_APIC_SPACE         (1 <<  3)
#define BX_CPUID_EXT1_ECX_ALT_MOV_CR8            (1 <<  4)
#define BX_CPUID_EXT1_ECX_LZCNT                  (1 <<  5)
#define BX_CPUID_EXT1_ECX_SSE4A                  (1 <<  6)
#define BX_CPUID_EXT1_ECX_MISALIGNED_SSE         (1 <<  7)
#define BX_CPUID_EXT1_ECX_PREFETCHW              (1 <<  8)
#define BX_CPUID_EXT1_ECX_OSVW                   (1 <<  9)
#define BX_CPUID_EXT1_ECX_IBS                    (1 << 10)
#define BX_CPUID_EXT1_ECX_XOP                    (1 << 11)
#define BX_CPUID_EXT1_ECX_SKINIT                 (1 << 12)
#define BX_CPUID_EXT1_ECX_WDT                    (1 << 13)
#define BX_CPUID_EXT1_ECX_RESERVED14             (1 << 14)
#define BX_CPUID_EXT1_ECX_LWP                    (1 << 15)
#define BX_CPUID_EXT1_ECX_FMA4                   (1 << 16)
#define BX_CPUID_EXT1_ECX_TCE                    (1 << 17)
#define BX_CPUID_EXT1_ECX_RESERVED18             (1 << 18)
#define BX_CPUID_EXT1_ECX_NODEID                 (1 << 19)
#define BX_CPUID_EXT1_ECX_RESERVED20             (1 << 20)
#define BX_CPUID_EXT1_ECX_TBM                    (1 << 21)
#define BX_CPUID_EXT1_ECX_TOPOLOGY_EXTENSIONS    (1 << 22)
#define BX_CPUID_EXT1_ECX_PERFCTR_EXT_CORE       (1 << 23)
#define BX_CPUID_EXT1_ECX_PERFCTR_EXT_NB         (1 << 24)
#define BX_CPUID_EXT1_ECX_RESERVED25             (1 << 25)
#define BX_CPUID_EXT1_ECX_DATA_BREAKPOINT_EXT    (1 << 26)
#define BX_CPUID_EXT1_ECX_PERF_TSC               (1 << 27)
#define BX_CPUID_EXT1_ECX_PERFCTR_EXT_L2I        (1 << 28)
#define BX_CPUID_EXT1_ECX_MONITORX_MWAITX        (1 << 29)
#define BX_CPUID_EXT1_ECX_CODEBP_ADDRMASK_EXT    (1 << 30)
#define BX_CPUID_EXT1_ECX_RESERVED31             (1 << 31)

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