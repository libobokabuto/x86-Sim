#include "bochs.h"
#ifndef BX_STANDALONE_DECODER
#include "cpu.h"
#endif

#include "instr.h"
#include "decoder.h"

extern int fetchDecode32(const Bit8u* fetchPtr, bool is_32, bxInstruction_c* i, unsigned remainingInPage); //42
#if BX_SUPPORT_X86_64
extern int fetchDecode64(const Bit8u* fetchPtr, bxInstruction_c* i, unsigned remainingInPage); //44
#endif