#pragma once
#if InstrumentICACHE
#define INC_ICACHE_STAT(stat) INC_CPU_STAT(stat)
#else
#define INC_ICACHE_STAT(stat)
#endif

#if InstrumentSMC
#define INC_SMC_STAT(stat) INC_CPU_STAT(stat)
#else
#define INC_SMC_STAT(stat)
#endif