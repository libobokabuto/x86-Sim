#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_AVX

extern softfloat_status_t mxcsr_to_softfloat_status_word(bx_mxcsr_t mxcsr);
extern void mxcsr_to_softfloat_status_word_imm_override(softfloat_status_t& status, Bit8u immb);

extern float32 approximate_rsqrt(float32 op);
extern float32 approximate_rcp(float32 op);

#include "softfloat-compare.h"
#include "simd_pfp.h"
#include "simd_int.h"

void BX_CPU_C::print_state_AVX(void)
{
    //BX_DEBUG(("MXCSR: 0x%08x", BX_MXCSR_REGISTER));
    for (int n = 0; n < BX_XMM_REGISTERS; n++) {
#if BX_SUPPORT_EVEX
        BxPackedZmmRegister vmm = BX_READ_AVX_REG(n);
        //BX_DEBUG(("VMM%02u: %08x%08x:%08x%08x:%08x%08x:%08x%08x:%08x%08x:%08x%08x:%08x%08x:%08x%08x", n,
            //vmm.zmm32u(15), vmm.zmm32u(14), vmm.zmm32u(13), vmm.zmm32u(12),
            //vmm.zmm32u(11), vmm.zmm32u(10), vmm.zmm32u(9), vmm.zmm32u(8),
            //vmm.zmm32u(7), vmm.zmm32u(6), vmm.zmm32u(5), vmm.zmm32u(4),
            //vmm.zmm32u(3), vmm.zmm32u(2), vmm.zmm32u(1), vmm.zmm32u(0)));
#else
        BxPackedYmmRegister vmm = BX_READ_YMM_REG(n);
        //BX_DEBUG(("VMM%02u: %08x%08x:%08x%08x:%08x%08x:%08x%08x", n,
           // vmm.ymm32u(7), vmm.ymm32u(6), vmm.ymm32u(5), vmm.ymm32u(4),
            //vmm.ymm32u(3), vmm.ymm32u(2), vmm.ymm32u(1), vmm.ymm32u(0)));
#endif
    }
}

#endif // BX_SUPPORT_AVX