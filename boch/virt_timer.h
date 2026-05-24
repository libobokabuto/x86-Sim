#pragma once
#define _BX_VIRT_TIMER_H

#include "pc_system.h"

#define BX_MAX_VIRTUAL_TIMERS (32)

class BOCHSAPI bx_virt_timer_c {
private:

    struct {
        bool inUse;         // Timer slot is in-use (currently registered).
        Bit64u  period;     // Timer periodocity in virtual useconds.
        Bit64u  timeToFire; // Time to fire next (in virtual useconds).
        bool active;        // 0=inactive, 1=active.
        bool continuous;    // 0=one-shot timer, 1=continuous periodicity.
        bool realtime;      // 0=standard timer, 1=use realtime mode
        bx_timer_handler_t funct;  // A callback function for when the
        //   timer fires.
        //   This function MUST return.
        void* this_ptr;            // The this-> pointer for C++ callbacks
        //   has to be stored as well.
        char id[BxMaxTimerIDLen]; // String ID of timer.
    } timer[BX_MAX_VIRTUAL_TIMERS];

    unsigned   numTimers;  // Number of currently allocated timers.

    struct {
        //Variables for the timer subsystem:
        Bit64u current_timers_time;
        Bit64u timers_next_event_time;
        Bit64u last_sequential_time;

        //Variables for the time sync subsystem:
        Bit64u virtual_next_event_time;
        Bit64u current_virtual_time;

        int system_timer_id;
    } s[2];

    bool in_timer_handler;

    // Local copy of IPS value
    Bit64u ips;

    bool init_done;

    //Real time variables:
    Bit64u last_real_time;
    Bit64u total_real_usec;
    Bit64u last_realtime_delta;
    Bit64u real_time_delay;
    //System time variables:
    Bit64u last_usec;
    Bit64u usec_per_second;
    Bit64u stored_delta;
    Bit64u last_system_usec;
    Bit64u em_last_realtime;
    //Virtual timer variables:
    Bit64u total_ticks;
    Bit64u last_realtime_ticks;
    Bit64u ticks_per_second;
    static const Bit64u NullTimerInterval;
    static void nullTimer(void* this_ptr);

    void periodic(Bit64u time_passed, bool mode);

    void next_event_time_update(bool mode);

    void advance_virtual_time(Bit64u time_passed, bool mode);

public:
    bx_virt_timer_c();
    virtual ~bx_virt_timer_c() {}

    Bit64u time_usec(bool mode);

    int  register_timer(void* this_ptr, bx_timer_handler_t handler,
        Bit32u useconds, bool continuous,
        bool active, bool realtime, const char* id);

    void activate_timer(unsigned timer_index, Bit32u useconds, bool continuous);

    void deactivate_timer(unsigned timer_index);

    void timer_handler(bool mode);
    void setup(void);

};

BOCHSAPI extern bx_virt_timer_c bx_virt_timer;
