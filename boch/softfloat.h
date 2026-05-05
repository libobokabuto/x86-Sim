#pragma once
#ifndef softfloat_h
#define softfloat_h 1

#include <stdbool.h>
#include <stdint.h>

struct softfloat_status_t
{
    uint8_t softfloat_roundingMode;
    int softfloat_exceptionFlags;
    int softfloat_exceptionMasks;
    int softfloat_suppressException;

    bool softfloat_denormals_are_zeros;
    bool softfloat_flush_underflow_to_zero;

    /*----------------------------------------------------------------------------
    | Rounding precision for 80-bit extended double-precision floating-point.
    | Valid values are 32, 64, and 80.
    *----------------------------------------------------------------------------*/
    uint8_t extF80_roundingPrecision;
};

#endif