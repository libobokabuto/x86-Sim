#pragma once
#ifndef BX_FPU_TRANSCENDENTAL_H
#define BX_FPU_TRANSCENDENTAL_H

#include "softfloat.h"

#include "softfloat-specialize.h"

extern floatx80 softfloat_propagateNaNExtF80UI(uint16_t uiA64, uint64_t uiA0, uint16_t uiB64, uint64_t uiB0, struct softfloat_status_t* status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/

int floatx80_remainder(floatx80 a, floatx80 b, floatx80& r, Bit64u& q, struct softfloat_status_t* status);
int floatx80_ieee754_remainder(floatx80 a, floatx80 b, floatx80& r, Bit64u& q, struct softfloat_status_t* status);

floatx80 f2xm1(floatx80 a, softfloat_status_t& status);
floatx80 fyl2x(floatx80 a, floatx80 b, softfloat_status_t& status);
floatx80 fyl2xp1(floatx80 a, floatx80 b, softfloat_status_t& status);
floatx80 fpatan(floatx80 a, floatx80 b, softfloat_status_t& status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision trigonometric functions.
*----------------------------------------------------------------------------*/

int fsincos(floatx80 a, floatx80* sin_a, floatx80* cos_a, softfloat_status_t& status);
int fsin(floatx80& a, softfloat_status_t& status);
int fcos(floatx80& a, softfloat_status_t& status);
int ftan(floatx80& a, softfloat_status_t& status);

/*-----------------------------------------------------------------------------
| Calculates the absolute value of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80& floatx80_abs(floatx80& reg)
{
    reg.signExp &= 0x7FFF;
    return reg;
}

/*-----------------------------------------------------------------------------
| Changes the sign of the extended double-precision floating-point value 'a'.
| The operation is performed according to the IEC/IEEE Standard for Binary
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80& floatx80_chs(floatx80& reg)
{
    reg.signExp ^= 0x8000;
    return reg;
}

/*-----------------------------------------------------------------------------
| Commonly used extended double-precision floating-point constants.
*----------------------------------------------------------------------------*/

extern const floatx80 Const_Z;
extern const floatx80 Const_1;

#endif