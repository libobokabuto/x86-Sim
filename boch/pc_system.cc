#define _CRT_SECURE_NO_WARNINGS
#include "bochs.h"
#include "cpu.h"
#include "iodev.h"
#include "debug.h"
#define MinAllowableTimerPeriod 1 //37
Bit32u BX_CPP_AttrRegparmN(2)
bx_pc_system_c::inp(Bit16u addr, unsigned io_len)
{  //107
    Bit32u ret = bx_devices.inp(addr, io_len);
    return ret;
}

void BX_CPP_AttrRegparmN(3)
bx_pc_system_c::outp(Bit16u addr, Bit32u value, unsigned io_len)
{ //118
    bx_devices.outp(addr, value, io_len);
}

void bx_pc_system_c::set_enable_a20(bool value)
{  //123
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
{  //163
#if BX_SUPPORT_A20
    return enable_a20;
#else
    return true;
#endif
}

int bx_pc_system_c::Reset(unsigned type)
{  //187
    set_enable_a20(1);
    BX_CPU(0)->reset(type);
    return 0;
}
int bx_pc_system_c::register_timer_ticks(void* this_ptr, bx_timer_handler_t funct,
    Bit64u ticks, bool continuous, bool active, const char* id)
{
    unsigned i;

    // If the timer frequency is rediculously low, make it more sane.
    // This happens when 'ips' is too low.
    if (ticks < MinAllowableTimerPeriod) {
        //BX_INFO(("register_timer_ticks: adjusting ticks of %llu to min of %u",
        //          ticks, MinAllowableTimerPeriod));
        ticks = MinAllowableTimerPeriod;
    }

    // search for new timer (i = 0 is reserved for NullTimer)
    for (i = 1; i < numTimers; i++) {
        if (timer[i].inUse == 0)
            break;
    }

    if (numTimers >= BX_MAX_TIMERS) {
        //BX_PANIC(("register_timer: too many registered timers"));
        return -1;
    }
#if BX_TIMER_DEBUG
    if (this_ptr == NULL)
        BX_PANIC(("register_timer_ticks: this_ptr is NULL!"));
    if (funct == NULL)
        BX_PANIC(("register_timer_ticks: funct is NULL!"));
#endif

    timer[i].inUse = 1;
    timer[i].period = ticks;
    timer[i].timeToFire = (ticksTotal + Bit64u(currCountdownPeriod - currCountdown)) + ticks;
    timer[i].active = active;
    timer[i].continuous = continuous;
    timer[i].funct = funct;
    timer[i].this_ptr = this_ptr;
    strncpy(timer[i].id, id, BxMaxTimerIDLen);
    timer[i].id[BxMaxTimerIDLen - 1] = 0; // Null terminate if not already.
    timer[i].param = 0;

    if (active) {
        if (ticks < Bit64u(currCountdown)) {
            // This new timer needs to fire before the current countdown.
            // Skew the current countdown and countdown period to be smaller
            // by the delta.
            currCountdownPeriod -= (currCountdown - Bit32u(ticks));
            currCountdown = Bit32u(ticks);
        }
    }

    //BX_DEBUG(("timer id %d registered for '%s'", i, id));
    // If we didn't find a free slot, increment the bound, numTimers.
    if (i == numTimers)
        numTimers++; // One new timer installed.

    // Return timer id.
    return i;
}

void bx_pc_system_c::countdownEvent(void)
{
    unsigned i, first = numTimers, last = 0;
    Bit64u   minTimeToFire;
    bool  triggered[BX_MAX_TIMERS];

    // The countdown decremented to 0.  We need to service all the active
    // timers, and invoke callbacks from those timers which have fired.
#if BX_TIMER_DEBUG
    if (currCountdown != 0)
        BX_PANIC(("countdownEvent: ticks!=0"));
#endif

    // Increment global ticks counter by number of ticks which have
    // elapsed since the last update.
    ticksTotal += Bit64u(currCountdownPeriod);
    minTimeToFire = (Bit64u)-1;

    for (i = 0; i < numTimers; i++) {
        triggered[i] = 0; // Reset triggered flag.
        if (timer[i].active) {
#if BX_TIMER_DEBUG
            if (ticksTotal > timer[i].timeToFire)
                BX_PANIC(("countdownEvent: ticksTotal > timeToFire[%u], D " FMT_LL "u", i,
                    timer[i].timeToFire - ticksTotal));
#endif
            if (ticksTotal == timer[i].timeToFire) {
                // This timer is ready to fire.
                triggered[i] = 1;

                if (timer[i].continuous == 0) {
                    // If triggered timer is one-shot, deactive.
                    timer[i].active = 0;
                }
                else {
                    // Continuous timer, increment time-to-fire by period.
                    timer[i].timeToFire += timer[i].period;
                    if (timer[i].timeToFire < minTimeToFire)
                        minTimeToFire = timer[i].timeToFire;
                }
                if (i < first) first = i;
                last = i;
            }
            else {
                // This timer is not ready to fire yet.
                if (timer[i].timeToFire < minTimeToFire)
                    minTimeToFire = timer[i].timeToFire;
            }
        }
    }

    // Calculate next countdown period.  We need to do this before calling
    // any of the callbacks, as they may call timer features, which need
    // to be advanced to the next countdown cycle.
    currCountdown = currCountdownPeriod =
        Bit32u(minTimeToFire - ticksTotal);

    for (i = first; i <= last; i++) {
        // Call requested timer function.  It may request a different
        // timer period or deactivate etc.
        if (triggered[i] && (timer[i].funct != NULL)) {
            triggeredTimer = i;
            timer[i].funct(timer[i].this_ptr);
            triggeredTimer = 0;
        }
    }
}

void bx_pc_system_c::deactivate_timer(unsigned i)
{ //563
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

