#pragma once

#define softfloat_types_h 1

#include <stdint.h>
#include "config.h"

/*----------------------------------------------------------------------------
| Software IEC/IEEE floating-point types.
*----------------------------------------------------------------------------*/
typedef uint16_t float16, bfloat16;
typedef uint32_t float32;
typedef uint64_t float64;

#ifdef BX_LITTLE_ENDIAN
struct uint128 { uint64_t v0, v64; };
struct uint64_extra { uint64_t extra, v; };
struct uint128_extra { uint64_t extra; struct uint128 v; };
#else
struct uint128 { uint64_t v64, v0; };
struct uint64_extra { uint64_t v, extra; };
struct uint128_extra { struct uint128 v; uint64_t extra; };
#endif

struct f16_t {
    uint16_t v;
#ifdef __cplusplus
    f16_t(uint16_t v16) : v(v16) {}
    operator uint16_t() const { return v; }
#endif
};

struct f32_t {
    uint32_t v;
#ifdef __cplusplus
    f32_t(uint32_t v32) : v(v32) {}
    operator uint32_t() const { return v; }
#endif
};

struct f64_t {
    uint64_t v;
#ifdef __cplusplus
    f64_t(uint64_t v64) : v(v64) {}
    operator uint64_t() const { return v; }
#endif
};

typedef uint128 float128_t;

#ifdef BX_LITTLE_ENDIAN
struct extFloat80M {
    uint64_t signif;
    uint16_t signExp;
};
#else
struct extFloat80M {
    uint16_t signExp;
    uint64_t signif;
};
#endif

typedef struct extFloat80M extFloat80_t, floatx80;