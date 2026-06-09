#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include <stdlib.h>

#define HW_RANDOM_GENERATOR_READY (1)

bool hw_rand_ready()
{
	return HW_RANDOM_GENERATOR_READY;
}

// provide a byte of data from Hardware Random Generator (TBD: implement as device)
Bit8u hw_rand8()
{
	return rand() & 0xff;     // hack using std C rand() function
}

// provide a 16-bit of data from Hardware Random Generator (TBD: implement as device)
Bit16u hw_rand16()
{
	Bit16u val_16 = 0;

	val_16 |= hw_rand8();
	val_16 <<= 8;
	val_16 |= hw_rand8();

	return val_16;
}

// provide a 32-bit of data from Hardware Random Generator (TBD: implement as device)
Bit32u hw_rand32()
{
	Bit32u val_32 = 0;

	val_32 |= hw_rand16();
	val_32 <<= 16;
	val_32 |= hw_rand16();

	return val_32;
}

// provide a 64-bit of data from Hardware Random Generator (TBD: implement as device)
Bit64u hw_rand64()
{
	Bit64u val_64 = 0;

	val_64 |= hw_rand32();
	val_64 <<= 32;
	val_64 |= hw_rand32();

	return val_64;
}