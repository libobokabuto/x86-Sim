#pragma once

#define BX_386_CPUID_DEFINITIONS_H

#include <assert.h>

#include "cpuid.h"

class i386_t : public bx_cpuid_t {
public:
    i386_t(BX_CPU_C* cpu) : bx_cpuid_t(cpu) {
        enable_cpu_extension(BX_ISA_X87);
    }
    virtual ~i386_t() {}

    // return CPU name
    virtual const char* get_name(void) const { return "i386"; }

    virtual void get_cpuid_leaf(Bit32u function, Bit32u subfunction, cpuid_function_t* leaf) const { assert(0); }

    virtual void dump_cpuid(void) const {}
};

bx_cpuid_t* create_i386_cpuid(BX_CPU_C* cpu) { return new i386_t(cpu); }
