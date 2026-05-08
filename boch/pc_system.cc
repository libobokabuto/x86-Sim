#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
#include "debug.h"
#define MinAllowableTimerPeriod 1 //37
Bit32u BX_CPP_AttrRegparmN(2)
bx_pc_system_c::inp(Bit16u addr, unsigned io_len)
{
    Bit32u ret = bx_devices.inp(addr, io_len);
    return ret;
}

void BX_CPP_AttrRegparmN(3)
bx_pc_system_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{
    bx_devices.outp(addr, value, io_len);
}

void bx_pc_system_c::set_enable_a20(bool value)
{
#if BX_SUPPORT_A20
    enable_a20 = value ? true : false;

    if (value) {
#if BX_CPU_LEVEL < 2
        a20_mask = 0xfffff;
#elif BX_CPU_LEVEL == 2
        a20_mask = 0xffffff;
#elif BX_PHY_ADDRESS_LONG
        a20_mask = BX_CONST64(0xffffffffffffffff);
#else
        a20_mask = 0xffffffff;
#endif
    }
    else {
#if BX_PHY_ADDRESS_LONG
        a20_mask = BX_CONST64(0xffffffffffefffff);
#else
        a20_mask = 0xffefffff;
#endif
    }
#else
    UNUSED(value);
    enable_a20 = true;
    a20_mask = (bx_phy_address)(~(bx_phy_address)0);
#endif
}

bool bx_pc_system_c::get_enable_a20(void)
{
#if BX_SUPPORT_A20
    return enable_a20;
#else
    return true;
#endif
}

int bx_pc_system_c::Reset(unsigned type)
{
    set_enable_a20(1);
    BX_CPU(0)->reset(type);
    return 0;
}

void bx_pc_system_c::deactivate_timer(unsigned i)
{
#if BX_TIMER_DEBUG
    if (i >= numTimers)
        BX_PANIC(("deactivate_timer: timer %u OOB", i));
    if (i == 0)
        BX_PANIC(("deactivate_timer: timer 0 is the nullTimer!"));
#endif

    timer[i].active = 0;
}
void bx_pc_system_c::activate_timer_ticks(unsigned i, Bit64u ticks, bool continuous)
{
    //474
#if BX_TIMER_DEBUG
    if (i >= numTimers)
        BX_PANIC(("activate_timer_ticks: timer %u OOB", i));
    if (i == 0)
        BX_PANIC(("activate_timer_ticks: timer 0 is the NullTimer!"));
    if (timer[i].period < MinAllowableTimerPeriod)
        BX_PANIC(("activate_timer_ticks: timer[%u].period of " FMT_LL "u < min of %u",
            i, timer[i].period, MinAllowableTimerPeriod));
#endif

    // If the timer frequency is rediculously low, make it more sane.
    // This happens when 'ips' is too low.
    if (ticks < MinAllowableTimerPeriod) {
        //BX_INFO(("activate_timer_ticks: adjusting ticks of %llu to min of %u",
        //          ticks, MinAllowableTimerPeriod));
        ticks = MinAllowableTimerPeriod;
    }

    timer[i].period = ticks;
    timer[i].timeToFire = (ticksTotal + Bit64u(currCountdownPeriod - currCountdown)) + ticks;
    timer[i].active = 1;
    timer[i].continuous = continuous;

    if (ticks < Bit64u(currCountdown)) {
        // This new timer needs to fire before the current countdown.
        // Skew the current countdown and countdown period to be smaller
        // by the delta.
        currCountdownPeriod -= (currCountdown - Bit32u(ticks));
        currCountdown = Bit32u(ticks);
    }
}
void bx_pc_system_c::activate_timer(unsigned i, Bit32u useconds, bool continuous)
{
    //508
    Bit64u ticks;

#if BX_TIMER_DEBUG
    if (i >= numTimers)
        BX_PANIC(("activate_timer: timer %u OOB", i));
    if (i == 0)
        BX_PANIC(("activate_timer: timer 0 is the nullTimer!"));
#endif

    // if useconds = 0, use default stored in period field
    // else set new period from useconds
    if (useconds == 0) {
        ticks = timer[i].period;
    }
    else {
        // convert useconds to number of ticks
        ticks = (Bit64u)(double(useconds) * m_ips);

        // If the timer frequency is rediculously low, make it more sane.
        // This happens when 'ips' is too low.
        if (ticks < MinAllowableTimerPeriod) {
            ticks = MinAllowableTimerPeriod;
        }

        timer[i].period = ticks;
    }

    activate_timer_ticks(i, ticks, continuous);
}

