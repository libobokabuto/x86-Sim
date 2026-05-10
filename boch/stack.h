#pragma once

 BX_CPP_INLINE void BX_CPP_AttrRegparmN(1)
BX_CPU_C::push_16(Bit16u value16)
{
#if BX_SUPPORT_X86_64
    if (long64_mode()) { /* StackAddrSize = 64 */
        stack_write_word(RSP - 2, value16);
        RSP -= 2;
    }
    else
#endif
        if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) { /* StackAddrSize = 32 */
            stack_write_word((Bit32u)(ESP - 2), value16);
            ESP -= 2;
        }
        else /* StackAddrSize = 16 */
        {
            stack_write_word((Bit16u)(SP - 2), value16);
            SP -= 2;
        }
}

#if BX_SUPPORT_CET  //130
 BX_CPP_INLINE void BX_CPP_AttrRegparmN(1) BX_CPU_C::shadow_stack_push_32(Bit32u value32)
 {
     shadow_stack_write_dword(SSP - 4, CPL, value32);
     SSP -= 4;
 }
#endif // BX_SUPPORT_X86_64  //166