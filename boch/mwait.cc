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
#include "apic.h"
#endif

#include "pc_system.h"

#include "ia_opcodes.h"

#if BX_SUPPORT_MONITOR_MWAIT
bool BX_CPU_C::is_monitor(bx_phy_address begin_addr, unsigned len)
{ //44
    if (!BX_CPU_THIS_PTR monitor.armed()) return false;

    bx_phy_address monitor_begin = BX_CPU_THIS_PTR monitor.monitor_addr;
    bx_phy_address monitor_end = monitor_begin + CACHE_LINE_SIZE - 1;

    bx_phy_address end_addr = begin_addr + len;
    if (begin_addr >= monitor_end || end_addr <= monitor_begin)
        return false;
    else
        return true;
}

void BX_CPU_C::check_monitor(bx_phy_address begin_addr, unsigned len)
{
    if (is_monitor(begin_addr, len)) wakeup_monitor();
}


void BX_CPU_C::wakeup_monitor(void)
{
    // wakeup from MWAIT state
    if (BX_CPU_THIS_PTR activity_state >= BX_ACTIVITY_STATE_MWAIT)
        BX_CPU_THIS_PTR activity_state = BX_ACTIVITY_STATE_ACTIVE;
    // clear monitor
    BX_CPU_THIS_PTR monitor.reset_monitor();
    // deactivate mwaitx timer if was active to avoid its redundant firing
    BX_CPU_THIS_PTR lapic->deactivate_mwaitx_timer();

}
#endif