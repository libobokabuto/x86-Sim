#pragma once

#define BX_486DX4_CPUID_DEFINITIONS_H

#if BX_CPU_LEVEL >= 4

#include "cpuid.h"

class i486dx4_t : public bx_cpuid_t {
public:
    i486dx4_t(BX_CPU_C* cpu);
    virtual ~i486dx4_t() {}

    virtual const char* get_name(void) const { return "i486dx4"; }

    virtual void get_cpuid_leaf(Bit32u function,
        Bit32u subfunction,
        cpuid_function_t* leaf) const;

    virtual void dump_cpuid(void) const;

private:
    void get_std_cpuid_leaf_1(cpuid_function_t* leaf) const;
};

extern bx_cpuid_t* create_i486dx4_cpuid(BX_CPU_C* cpu);

#endif
