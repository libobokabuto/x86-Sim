#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#include "cpuid.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_SVM
#include "svm.h"
#endif
#include "pc_system.h"
#include "ia_opcodes.h"


void BX_CPU_C::shutdown(void)
{ //136
#if BX_SUPPORT_SVM
	if (BX_CPU_THIS_PTR in_svm_guest) {
		if (SVM_INTERCEPT(SVM_INTERCEPT0_SHUTDOWN)) Svm_Vmexit(SVM_VMEXIT_SHUTDOWN);
	}
#endif

	enter_sleep_state(BX_ACTIVITY_STATE_SHUTDOWN);

	longjmp(BX_CPU_THIS_PTR jmp_buf_env, 1); // go back to main decode loop
}

void BX_CPU_C::enter_sleep_state(unsigned state)
{  //149
	switch (state) {
	case BX_ACTIVITY_STATE_ACTIVE:
		//BX_ASSERT(0); // should not be used for entering active CPU state
		break;

	case BX_ACTIVITY_STATE_HLT:
		break;

	case BX_ACTIVITY_STATE_WAIT_FOR_SIPI:
		mask_event(BX_EVENT_INIT | BX_EVENT_SMI | BX_EVENT_NMI); // FIXME: all events should be masked
		// fall through - mask interrupts as well

	case BX_ACTIVITY_STATE_SHUTDOWN:
		BX_CPU_THIS_PTR clear_IF(); // masking interrupts
		break;

	case BX_ACTIVITY_STATE_MWAIT:
	case BX_ACTIVITY_STATE_MWAIT_IF:
		break;

	default:
		//BX_PANIC(("enter_sleep_state: unknown state %d", state));
		break;
	}

	// artificial trap bit, why use another variable.
	BX_CPU_THIS_PTR activity_state = state;
	BX_CPU_THIS_PTR async_event = 1; // so processor knows to check
	// Execution completes.  The processor will remain in a sleep
	// state until one of the wakeup conditions is met.

	BX_INSTR_HLT(BX_CPU_ID);

#if BX_DEBUGGER
	if (bx_dbg.debugger_active) {
		//bx_dbg_halt(BX_CPU_ID);
	}
#endif

#if BX_USE_IDLE_HACK
	bx_gui->sim_is_idle();
#endif
}



void BX_CPU_C::handleCpuModeChange(void)
{
	unsigned mode = BX_CPU_THIS_PTR cpu_mode;

#if BX_SUPPORT_X86_64
	if (BX_CPU_THIS_PTR efer.get_LMA()) {
		if (!BX_CPU_THIS_PTR cr0.get_PE()) {
			//BX_PANIC(("change_cpu_mode: EFER.LMA is set when CR0.PE=0 !"));
		}
		if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l) {
			BX_CPU_THIS_PTR cpu_mode = BX_MODE_LONG_64;
		}
		else {
			BX_CPU_THIS_PTR cpu_mode = BX_MODE_LONG_COMPAT;
			// clear upper part of RIP/RSP when leaving 64-bit long mode
			BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RIP);
			BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSP);
		}

		// switching between compatibility and long64 mode also affect SS.BASE
		// which is always zero in long64 mode
		invalidate_stack_cache();
	}
	else
#endif
	{
		if (BX_CPU_THIS_PTR cr0.get_PE()) {
			if (BX_CPU_THIS_PTR get_VM()) {
				BX_CPU_THIS_PTR cpu_mode = BX_MODE_IA32_V8086;
				CPL = 3;
			}
			else
				BX_CPU_THIS_PTR cpu_mode = BX_MODE_IA32_PROTECTED;
		}
		else {
			BX_CPU_THIS_PTR cpu_mode = BX_MODE_IA32_REAL;

			// CS segment in real mode always allows full access
			BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
			BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1;  /* data/code segment */
			BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type = BX_DATA_READ_WRITE_ACCESSED;

			CPL = 0;
		}
	}

	updateFetchModeMask();

#if BX_CPU_LEVEL >= 6
#if BX_SUPPORT_AVX
	handleAvxModeChange(); /* protected mode reloaded */
#endif
#endif

	// re-initialize protection keys
#if BX_SUPPORT_PKEYS
	set_PKeys(BX_CPU_THIS_PTR pkru, BX_CPU_THIS_PTR pkrs);
#endif

#if BX_DEBUGGER
	if (bx_dbg.debugger_active) {
		// assert magic async_event to stop trace execution
		BX_CPU_THIS_PTR async_event |= BX_ASYNC_EVENT_STOP_TRACE;
	}
#endif

	if (mode != BX_CPU_THIS_PTR cpu_mode) {
		//BX_DEBUG(("%s activated", cpu_mode_string(BX_CPU_THIS_PTR cpu_mode)));
#if BX_DEBUGGER
		if (bx_dbg.debugger_active) {
			if (BX_CPU_THIS_PTR mode_break) {
				//BX_CPU_THIS_PTR stop_reason = STOP_MODE_BREAK_POINT;
				//bx_debug_break(); // trap into debugger
			}
		}
#endif
	}
}


#if BX_CPU_LEVEL >= 4
void BX_CPU_C::handleAlignmentCheck(void)
{
	if (CPL == 3 && BX_CPU_THIS_PTR cr0.get_AM() && BX_CPU_THIS_PTR get_AC()) {
#if BX_SUPPORT_ALIGNMENT_CHECK == 0
		BX_PANIC(("WARNING: Alignment check (#AC exception) was not compiled in !"));
#else
		BX_CPU_THIS_PTR alignment_check_mask = 0xF;
#endif
	}
#if BX_SUPPORT_ALIGNMENT_CHECK
	else {
		BX_CPU_THIS_PTR alignment_check_mask = 0;
	}
#endif
}
#endif

void BX_CPU_C::handleFpuMmxModeChange(void)
{
	if (BX_CPU_THIS_PTR cr0.get_EM() || BX_CPU_THIS_PTR cr0.get_TS())
		clear_fpu_mmx_ok();
	else
		set_fpu_mmx_ok();

	updateFetchModeMask(); /* FPU_MMX_OK changed */
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxError(bxInstruction_c* i)
{

}


void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoFPU(bxInstruction_c* i)
{
	//456
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoMMX(bxInstruction_c* i)
{
	//466
}
#if BX_CPU_LEVEL >= 6
void BX_CPU_C::handleSseModeChange(void)
{
	if (BX_CPU_THIS_PTR cr0.get_TS()) {
		clear_sse_ok();
	}
	else {
		if (BX_CPU_THIS_PTR cr0.get_EM() || !BX_CPU_THIS_PTR cr4.get_OSFXSR())
			clear_sse_ok();
		else
			set_sse_ok();
	}

	updateFetchModeMask(); /* SSE_OK changed */
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoSSE(bxInstruction_c* i)
{
	//495
}
#endif

#if BX_SUPPORT_AVX
void BX_CPU_C::handleAvxModeChange(void)
{  //509
	if (BX_CPU_THIS_PTR cr0.get_TS()) {
		clear_avx_ok();
	}
	else {
		if (!protected_mode() || !BX_CPU_THIS_PTR cr4.get_OSXSAVE() ||
			(~BX_CPU_THIS_PTR xcr0.val32 & (BX_XCR0_SSE_MASK | BX_XCR0_YMM_MASK)) != 0) {
			clear_avx_ok();
		}
		else {
			set_avx_ok();

#if BX_SUPPORT_EVEX
			if ((~BX_CPU_THIS_PTR xcr0.val32 & BX_XCR0_OPMASK_MASK) != 0) {
				clear_opmask_ok();
				clear_evex_ok();
			}
			else {
				set_opmask_ok();

				if ((~BX_CPU_THIS_PTR xcr0.val32 & (BX_XCR0_ZMM_HI256_MASK | BX_XCR0_HI_ZMM_MASK)) != 0)
					clear_evex_ok();
				else
					set_evex_ok();
			}
#endif
		}
	}

#if BX_SUPPORT_AMX
	if (!long64_mode() || !BX_CPU_THIS_PTR cr4.get_OSXSAVE() ||
		(~BX_CPU_THIS_PTR xcr0.val32 & (BX_XCR0_XTILECFG_MASK | BX_XCR0_XTILEDATA_MASK)) != 0)
		clear_amx_ok();
	else
		set_amx_ok();
#endif

	updateFetchModeMask(); /* AVX_OK changed */
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoAVX(bxInstruction_c* i)
{
	//550
}
#endif
void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoOpMask(bxInstruction_c* i)
{
	//568
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::BxNoEVEX(bxInstruction_c* i)
{
	//584
}

void BX_CPU_C::handleCpuContextChange(void)
{
	TLB_flush();

	invalidate_prefetch_q();
	invalidate_stack_cache();

	handleInterruptMaskChange();

#if BX_CPU_LEVEL >= 4
	handleAlignmentCheck();
#endif

	handleCpuModeChange();

	handleFpuMmxModeChange();
#if BX_CPU_LEVEL >= 6
	handleSseModeChange();
#if BX_SUPPORT_AVX
	handleAvxModeChange();
#endif
#endif


}

#if BX_CPU_LEVEL >= 5

#include "wide_int.h"

Bit64u BX_CPU_C::get_TSC(void)
{
	Bit64u tsc = bx_pc_system.time_ticks() + BX_CPU_THIS_PTR tsc_adjust;
	return tsc;
}

Bit64u BX_CPU_C::get_Virtual_TSC()
{
	Bit64u tsc = BX_CPU_THIS_PTR get_TSC();
#if BX_SUPPORT_VMX
	if (BX_CPU_THIS_PTR in_vmx_guest) {
		if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1.TSC_OFFSET() && BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.TSC_SCALING()) {
			// RDTSC first computes the product of the value of the IA32_TIME_STAMP_COUNTER MSR and
			// the value of the TSC multiplier. It then shifts the value of the product right 48 bits and loads 
			// EAX:EDX with <the sum of that shifted value and the value of the TSC offset>.
			Bit128u product_128;
			long_mul(&product_128, tsc, BX_CPU_THIS_PTR vmcs.tsc_multiplier);
			tsc = (product_128.lo >> 48) | (product_128.hi << 16);   // tsc = (uint64) (long128(tsc_value * tsc_multiplier) >> 48);
		}
	}
#endif
#if BX_SUPPORT_VMX || BX_SUPPORT_SVM
	tsc += BX_CPU_THIS_PTR tsc_offset;    // BX_CPU_THIS_PTR tsc_offset = 0 if not in VMX or SVM guest
#endif
	return tsc;
}

#if BX_SUPPORT_VMX
Bit64u BX_CPU_C::compute_physical_TSC_delay(Bit64u tsc_delay)
{
	if (BX_CPU_THIS_PTR in_vmx_guest) {
		if (BX_CPU_THIS_PTR vmcs.vmexec_ctrls1.TSC_OFFSET() && BX_CPU_THIS_PTR vmcs.vmexec_ctrls2.TSC_SCALING()) {
			// The virtual delay is multiplied by 2^48 (using a shift) to produce a 128-bit 
			// integer. That product is then divided by the TSC multiplier to produce a 64-bit integer.
			// The physical delay is that quotient.
			Bit128u product128, quotient;
			product128.hi = tsc_delay >> 16;
			product128.lo = tsc_delay << 48;
			long_div(&quotient, &tsc_delay /*just use it as temp to be destroyed*/, &product128, BX_CPU_THIS_PTR vmcs.tsc_multiplier);
			//BX_ASSERT(quotient.hi == 0);
			tsc_delay = quotient.lo;                // tsc = Bit128(tsc_value << 48) / tsc_multiplier
		}
	}
	return tsc_delay;
}
#endif


void BX_CPU_C::set_TSC(Bit64u newval)
{
	// compute the correct setting of tsc_adjust so that a get_TSC()
	// will return newval
	BX_CPU_THIS_PTR tsc_adjust = newval - bx_pc_system.time_ticks();

	// verify
	//BX_ASSERT(get_TSC() == newval);
	
}
#endif // BX_CPU_LEVEL >= 5