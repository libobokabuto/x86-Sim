#pragma once
#define BX_MAX_TIMERS 64
#define BX_NULL_TIMER_HANDLE 10000 //27
typedef void (*bx_timer_handler_t)(void*);
class BOCHSAPI bx_pc_system_c;

BOCHSAPI extern class bx_pc_system_c bx_pc_system; //31

class BOCHSAPI bx_pc_system_c { //37
private:

	struct {
		bool inUse;      // Timer slot is in-use (currently registered).
		Bit64u  period;     // Timer periodocity in cpu ticks.
		Bit64u  timeToFire; // Time to fire next (in absolute ticks).
		bool active;     // 0=inactive, 1=active.
		bool continuous; // 0=one-shot timer, 1=continuous periodicity.
		bx_timer_handler_t funct;  // A callback function for when the
		//   timer fires.
		void* this_ptr;            // The this-> pointer for C++ callbacks
		//   has to be stored as well.
#define BxMaxTimerIDLen 32
		char id[BxMaxTimerIDLen];  // String ID of timer.
		Bit32u param;              // Device-specific value assigned to timer (optional)
	} timer[BX_MAX_TIMERS];

	unsigned   numTimers;
	unsigned   triggeredTimer;
	struct {
		Bit32u     currCountdown;
		Bit32u     currCountdownPeriod;
		Bit64u     ticksTotal;
	};

#if !defined(PROVIDE_M_IPS)
	// This is the emulator speed, as measured in millions of
	// x86 instructions per second that it can emulate on some hypothetically
	// nomimal workload.
	double     m_ips; // Millions of Instructions Per Second
#endif
	void   countdownEvent(void);
public:
	void   activate_timer(unsigned timer_index, Bit32u useconds, bool continuous); //97
	void   deactivate_timer(unsigned timer_index); //99
	static BX_CPP_INLINE void tick1(void) {  //106
		if (--bx_pc_system.currCountdown == 0) {
			bx_pc_system.countdownEvent();
		}
	}
	int register_timer_ticks(void* this_ptr, bx_timer_handler_t, Bit64u ticks,
		bool continuous, bool active, const char* id);
	void activate_timer_ticks(unsigned index, Bit64u instructions, bool continuous); //125
	bool enable_a20; //157
	bx_phy_address a20_mask; //166
	
	void set_enable_a20(bool value);
	bool get_enable_a20(void);
	int Reset(unsigned type);
	static BX_CPP_INLINE Bit64u time_ticks() {
		return bx_pc_system.ticksTotal +
			Bit64u(bx_pc_system.currCountdownPeriod - bx_pc_system.currCountdown);
	}
	Bit32u  inp(Bit16u addr, unsigned io_len) BX_CPP_AttrRegparmN(2);
	void    outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3);

}; //189

#define BX_TICK1()                  bx_pc_system.tick1()
#define BX_SET_ENABLE_A20(enabled)  bx_pc_system.set_enable_a20(enabled)
#define BX_GET_ENABLE_A20()         bx_pc_system.get_enable_a20()

#if BX_SUPPORT_A20
#  define A20ADDR(x)                ((bx_phy_address)(x) & bx_pc_system.a20_mask)
#else
#  define A20ADDR(x)                ((bx_phy_address)(x))
#endif