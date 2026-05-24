#include "bochs.h"
#include "cpu.h"
#include "i486dx4.h"

#if BX_CPU_LEVEL >= 4

#define LOG_THIS cpu->

i486dx4_t::i486dx4_t(BX_CPU_C* cpu) : bx_cpuid_t(cpu)
{
	enable_cpu_extension(BX_ISA_X87);
	enable_cpu_extension(BX_ISA_486);
	enable_cpu_extension(BX_ISA_VME);
}

void i486dx4_t::get_cpuid_leaf(Bit32u function, Bit32u subfunction, cpuid_function_t* leaf) const
{
    switch (function) {
    case 0x00000000:
        get_leaf_0(0x1, "GenuineIntel", leaf);
        return;
    case 0x00000001:
    default:
        get_std_cpuid_leaf_1(leaf);
        return;
    }
}

void i486dx4_t::get_std_cpuid_leaf_1(cpuid_function_t* leaf) const
{
    leaf->eax = 0x00000480;
    leaf->ebx = 0;
    leaf->ecx = 0;
    leaf->edx = get_std_cpuid_leaf_1_edx();
}

void i486dx4_t::dump_cpuid(void) const
{
    bx_cpuid_t::dump_cpuid(0x1, 0);
}

bx_cpuid_t* create_i486dx4_cpuid(BX_CPU_C* cpu) { return new i486dx4_t(cpu); }

#endif