#pragma once

#define InstrumentICACHE 0
#define InstrumentTLB 0
#define InstrumentTLBFlush 0
#define InstrumentStackPrefetch 0
#define InstrumentSMC 0

struct bx_cpu_statistics
{
	Bit64u tlbLookups;
	Bit64u tlbExecuteLookups;
	Bit64u tlbWriteLookups;
	Bit64u tlbMisses;
	Bit64u tlbExecuteMisses;
	Bit64u tlbWriteMisses;

	// tlb flush statistics
	Bit64u tlbGlobalFlushes;
	Bit64u tlbNonGlobalFlushes;

	// stack prefetch statistics
	Bit64u stackPrefetch;

	// self modifying code statistics
	Bit64u smc;
	
};

#if InstrumentICACHE
#define INC_ICACHE_STAT(stat) INC_CPU_STAT(stat)
#else
#define INC_ICACHE_STAT(stat)
#endif

#if InstrumentStackPrefetch
#define INC_STACK_PREFETCH_STAT(stat) INC_CPU_STAT(stat)
#else
#define INC_STACK_PREFETCH_STAT(stat)
#endif

#if InstrumentSMC
#define INC_SMC_STAT(stat) INC_CPU_STAT(stat)
#else
#define INC_SMC_STAT(stat)
#endif