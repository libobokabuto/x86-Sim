#pragma once
#define BX_COREI7_HASWELL_4770_CPUID_DEFINITIONS_H

#if BX_SUPPORT_X86_64 && BX_SUPPORT_AVX

#include "cpuid.h"

class corei7_haswell_4770_t : public bx_cpuid_t {
public:
	corei7_haswell_4770_t(BX_CPU_C* cpu);
	virtual ~corei7_haswell_4770_t() {}

	// return CPU name
	virtual const char* get_name(void) const { return "corei7_haswell_4770"; }

#if BX_SUPPORT_VMX >= 2
	virtual Bit32u get_vmx_extensions_bitmask(void) const;
#endif

	virtual void get_cpuid_leaf(Bit32u function, Bit32u subfunction, cpuid_function_t* leaf) const;

	virtual void dump_cpuid(void) const;

private:
	Bit32u max_std_leaf;
	Bit32u max_ext_leaf;

	void get_std_cpuid_leaf_1(cpuid_function_t* leaf) const;
	void get_std_cpuid_leaf_2(cpuid_function_t* leaf) const;
	void get_std_cpuid_leaf_4(Bit32u subfunction, cpuid_function_t* leaf) const;
	void get_std_cpuid_leaf_7(Bit32u subfunction, cpuid_function_t* leaf) const;
	void get_std_cpuid_leaf_A(cpuid_function_t* leaf) const;

	void get_ext_cpuid_leaf_1(cpuid_function_t* leaf) const;
};

extern bx_cpuid_t* create_corei7_haswell_4770_cpuid(BX_CPU_C* cpu);

#endif // BX_SUPPORT_X86_64 && BX_SUPPORT_AVX


