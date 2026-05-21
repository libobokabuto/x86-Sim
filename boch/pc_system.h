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

	// Current countdown ticks value (decrements to 0).
	Bit32u     currCountdownPeriod; // Length of current countdown period.
	Bit64u     ticksTotal; // Num ticks total since start of emulator execution.
	Bit64u     lastTimeUsec; // Last sequentially read time in usec.
	Bit64u     usecSinceLast; // Number of useconds claimed since then.
	
	
	static const Bit64u NullTimerInterval;
	static void nullTimer(void* this_ptr);
#if !defined(PROVIDE_M_IPS)
	// This is the emulator speed, as measured in millions of
	// x86 instructions per second that it can emulate on some hypothetically
	// nomimal workload.
	double     m_ips; // Millions of Instructions Per Second
#endif
	void   countdownEvent(void);

public:

	Bit32u     currCountdown;
	void   initialize(Bit32u ips);
	int    register_timer(void* this_ptr, bx_timer_handler_t, Bit32u useconds,
		bool continuous, bool active, const char* id);
	void   setTimerParam(unsigned timerID, Bit32u param);//95
	void   activate_timer(unsigned timer_index, Bit32u useconds, bool continuous); //97

	void   deactivate_timer(unsigned timer_index); //99
	
	unsigned triggeredTimerID(void) {
		return triggeredTimer;
	}

	Bit32u triggeredTimerParam(void) {
		return timer[triggeredTimer].param;
	}

	static BX_CPP_INLINE void tick1(void) {  //106
		if (--bx_pc_system.currCountdown == 0) {
			bx_pc_system.countdownEvent();
		}
	}

	static BX_CPP_INLINE void tickn(Bit32u n) {
		while (n >= bx_pc_system.currCountdown) {
			n -= bx_pc_system.currCountdown;
			bx_pc_system.currCountdown = 0;
			bx_pc_system.countdownEvent();
			// bx_pc_system.currCountdown is adjusted to new value by countdownevent().
		}
		// 'n' is not (or no longer) >= the countdown size.  We can just decrement
		// the remaining requested ticks and continue.
		bx_pc_system.currCountdown -= n;
	}

	int register_timer_ticks(void* this_ptr, bx_timer_handler_t, Bit64u ticks,
		bool continuous, bool active, const char* id);
	void activate_timer_ticks(unsigned index, Bit64u instructions, bool continuous); //125
	
	

	static BX_CPP_INLINE Bit64u time_ticks() {
		return bx_pc_system.ticksTotal +
			Bit64u(bx_pc_system.currCountdownPeriod - bx_pc_system.currCountdown);
	}

	static BX_CPP_INLINE Bit32u  getNumCpuTicksLeftNextEvent(void) {
		return bx_pc_system.currCountdown;
	}



	bool HRQ;

	bool enable_a20; //157

	bx_phy_address a20_mask; //166

	volatile bool kill_bochs_request;
	
	

	void set_HRQ(bool val);  // set the Hold ReQuest line

	void raise_INTR(void);
	void clear_INTR(void);

	int Reset(unsigned type);
	Bit8u  IAC(void);

	bx_pc_system_c();

	Bit32u  inp(Bit16u addr, unsigned io_len) BX_CPP_AttrRegparmN(2);
	void    outp(Bit16u addr, Bit32u value, unsigned io_len) BX_CPP_AttrRegparmN(3);
	void set_enable_a20(bool value);
	bool get_enable_a20(void);
	
	
}; //189

#define BX_TICK1()                  bx_pc_system.tick1()
#define BX_TICKN(n)                 bx_pc_system.tickn(n)
#define BX_INTR                     bx_pc_system.INTR
#define BX_RAISE_INTR()             bx_pc_system.raise_INTR()
#define BX_CLEAR_INTR()             bx_pc_system.clear_INTR()

#define BX_SET_ENABLE_A20(enabled)  bx_pc_system.set_enable_a20(enabled)
#define BX_GET_ENABLE_A20()         bx_pc_system.get_enable_a20()
#define BX_HRQ                      bx_pc_system.HRQ

#if BX_SUPPORT_A20
#  define A20ADDR(x)                ((bx_phy_address)(x) & bx_pc_system.a20_mask)
#else
#  define A20ADDR(x)                ((bx_phy_address)(x))
#endif