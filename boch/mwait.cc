#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "siminterface.h"
#include "param_names.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR
#if BX_SUPPORT_SVM
#include "svm.h"
#endif

#if BX_SUPPORT_APIC

#endif

#include "pc_system.h"

#include "ia_opcodes.h"


bool BX_CPU_C::is_monitor(bx_phy_address begin_addr, unsigned len)
{
    if (!BX_CPU_THIS_PTR monitor.armed()) return false;

    bx_phy_address monitor_begin = BX_CPU_THIS_PTR monitor.monitor_addr;
    bx_phy_address monitor_end = monitor_begin + CACHE_LINE_SIZE - 1;

    bx_phy_address end_addr = begin_addr + len;
    if (begin_addr >= monitor_end || end_addr <= monitor_begin)
        return false;
    else
        return true;
}