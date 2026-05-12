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


 BX_CPP_INLINE Bit16u BX_CPU_C::pop_16(void)
 {  //81
     Bit16u value16;

#if BX_SUPPORT_X86_64
     if (long64_mode()) { /* StackAddrSize = 64 */
         value16 = stack_read_word(RSP);
         RSP += 2;
     }
     else
#endif
         if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) { /* StackAddrSize = 32 */
             value16 = stack_read_word(ESP);
             ESP += 2;
         }
         else { /* StackAddrSize = 16 */
             value16 = stack_read_word(SP);
             SP += 2;
         }

     return value16;
 }


#if BX_SUPPORT_CET  //130
 BX_CPP_INLINE void BX_CPP_AttrRegparmN(1) BX_CPU_C::shadow_stack_push_32(Bit32u value32)
 {
     shadow_stack_write_dword(SSP - 4, CPL, value32);
     SSP -= 4;
 }

 BX_CPP_INLINE Bit32u BX_CPU_C::shadow_stack_pop_32(void)
 {  //152
     Bit32u value32 = shadow_stack_read_dword(SSP, CPL);
     SSP += 4;
     return value32;
 }

#endif // BX_SUPPORT_X86_64  //166