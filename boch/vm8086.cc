#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

bool BX_CPU_C::v86_redirect_interrupt(Bit8u vector)
{//203
#if BX_CPU_LEVEL >= 5
    if (BX_CPU_THIS_PTR cr4.get_VME())
    {
        bx_address tr_base = BX_CPU_THIS_PTR tr.cache.u.segment.base;
        if (BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled < 103) {
            //BX_ERROR(("v86_redirect_interrupt(): TR.limit < 103 in VME"));
            exception(BX_GP_EXCEPTION, 0);
        }

        Bit32u io_base = system_read_word(tr_base + 102), offset = io_base - 32 + (vector >> 3);
        if (offset > BX_CPU_THIS_PTR tr.cache.u.segment.limit_scaled) {
            //BX_ERROR(("v86_redirect_interrupt(): failed to fetch VME redirection bitmap"));
            exception(BX_GP_EXCEPTION, 0);
        }

        Bit8u vme_redirection_bitmap = system_read_byte(tr_base + offset);
        if (!(vme_redirection_bitmap & (1 << (vector & 7))))
        {
            // redirect interrupt through virtual-mode idt
            Bit16u temp_flags = (Bit16u)read_eflags();

            Bit16u temp_CS = system_read_word(vector * 4 + 2);
            Bit16u temp_IP = system_read_word(vector * 4);

            if (BX_CPU_THIS_PTR get_IOPL() < 3) {
                temp_flags |= EFlagsIOPLMask;
                if (BX_CPU_THIS_PTR get_VIF())
                    temp_flags |= EFlagsIFMask;
                else
                    temp_flags &= ~EFlagsIFMask;
            }

            Bit16u old_IP = IP;
            Bit16u old_CS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;

            push_16(temp_flags);
            // push return address onto new stack
            push_16(old_CS);
            push_16(old_IP);

            load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], (Bit16u)temp_CS);
            EIP = temp_IP;

            BX_CPU_THIS_PTR clear_TF();
            BX_CPU_THIS_PTR clear_RF();
            if (BX_CPU_THIS_PTR get_IOPL() == 3)
                BX_CPU_THIS_PTR clear_IF();
            else
                BX_CPU_THIS_PTR clear_VIF();

            return true;
        }
    }
#endif
    // interrupt is not redirected or VME is OFF
    if (BX_CPU_THIS_PTR get_IOPL() < 3)
    {
        //BX_DEBUG(("v86_redirect_interrupt(): interrupt cannot be redirected, generate #GP(0)"));
        exception(BX_GP_EXCEPTION, 0);
    }

    return false;
}
