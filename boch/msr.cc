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


#if BX_CPU_LEVEL >= 5 //45
void BX_CPU_C::init_MSRs()
{
	//48
}
#endif // BX_CPU_LEVEL >= 5 //658

#if BX_CPU_LEVEL >= 6  //704
bool isMemTypeValidMTRR(unsigned memtype)
{
    switch (memtype) {
    case BX_MEMTYPE_UC:
    case BX_MEMTYPE_WC:
    case BX_MEMTYPE_WT:
    case BX_MEMTYPE_WP:
    case BX_MEMTYPE_WB:
        return true;
    default:
        return false;
    }
}
#endif //743