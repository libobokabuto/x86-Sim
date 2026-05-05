#include "bochs.h"
#include "cpu.h"
#include "siminterface.h"
#include "cpuid.h"


bool bx_cpuid_t::support_avx10_512() const {
	//404
	return is_cpu_extension_supported(BX_ISA_AVX512) || is_cpu_extension_supported(BX_ISA_AVX10_VL512);
}