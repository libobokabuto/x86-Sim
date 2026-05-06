#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"

#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#if BX_SUPPORT_APIC

#endif

#include "ia_opcodes.h"


#if BX_CPU_LEVEL >= 5
void BX_CPU_C::init_MSRs()
{
	//48
}
#endif // BX_CPU_LEVEL >= 5