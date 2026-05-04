#pragma once
#include "bochs.h"
#include "cpu.h"

BX_CPU_C::BX_CPU_C(unsigned id) : bx_cpuid(id)
#if BX_CPU_LEVEL >= 4
, cpuid(NULL)
#endif
{
	//51ÐÐ
}

#include "cpuid.h"

void BX_CPU_C::initialize(void)
{
	//178
}
BX_CPU_C::~BX_CPU_C()
{
	//826
}