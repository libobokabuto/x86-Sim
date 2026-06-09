#include <assert.h>

#include "softfloat.h"

//                            2         3         4               n
// f(x) ~ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//         0    1         2         3         4               n
//
//          --       2k                --        2k+1
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//   f(x) ~ [ p(x) + x * q(x) ]
//

float128_t EvalPoly(float128_t x, const float128_t* arr, int n, softfloat_status_t& status)
{
    float128_t r = arr[--n];

    do {
        r = f128_mulAdd(r, x, arr[--n], 0, &status);
        //      r = f128_mul(r, x, &status);
        //      r = f128_add(r, arr[--n], &status);

    } while (n > 0);

    return r;
}

//                  2         4         6         8               2n
// f(x) ~ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//         0    1         2         3         4               n
//
//          --       4k                --        4k+2
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//                    2
//   f(x) ~ [ p(x) + x * q(x) ]
//

float128_t EvenPoly(float128_t x, const float128_t* arr, int n, softfloat_status_t& status)
{
    return EvalPoly(f128_mul(x, x, &status), arr, n, status);
}

//                        3         5         7         9               2n+1
// f(x) ~ (C * x) + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//          0         1         2         3         4               n
//                        2         4         6         8               2n
//      = x * [ C + (C * x) + (C * x) + (C * x) + (C * x) + ... + (C * x)
//               0    1         2         3         4               n
//
//          --       4k                --        4k+2
//   p(x) = >  C  * x           q(x) = >  C   * x
//          --  2k                     --  2k+1
//
//                        2
//   f(x) ~ x * [ p(x) + x * q(x) ]
//

float128_t OddPoly(float128_t x, const float128_t* arr, int n, softfloat_status_t& status)
{
    return f128_mul(x, EvenPoly(x, arr, n, status), &status);
}
