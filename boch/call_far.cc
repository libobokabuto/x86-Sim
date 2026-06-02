#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

void BX_CPP_AttrRegparmN(3) BX_CPU_C::call_protected(bxInstruction_c* i, Bit16u cs_raw, bx_address disp)
{
    bx_selector_t cs_selector;
    Bit32u dword1, dword2;
    bx_descriptor_t cs_descriptor;

    if ((cs_raw & 0xfffc) == 0) {
        exception(BX_GP_EXCEPTION, 0);
    }

    parse_selector(cs_raw, &cs_selector);
    fetch_raw_descriptor(&cs_selector, &dword1, &dword2, BX_GP_EXCEPTION);
    parse_descriptor(dword1, dword2, &cs_descriptor);

    if (cs_descriptor.valid == 0) {
        exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
    }

    if (cs_descriptor.segment)
    {
        check_cs(&cs_descriptor, cs_raw, BX_SELECTOR_RPL(cs_raw), CPL);

#if BX_SUPPORT_CET
        bx_address temp_LIP = get_laddr(BX_SEG_REG_CS, RIP);
        Bit16u old_CS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
#endif

#if BX_SUPPORT_X86_64
        if (long_mode() && cs_descriptor.u.segment.l) {
            Bit64u temp_rsp = RSP;
            if (i->os64L()) {
                write_new_stack_qword(temp_rsp - 8, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_qword(temp_rsp - 16, cs_descriptor.dpl, RIP);
                temp_rsp -= 16;
            }
            else if (i->os32L()) {
                write_new_stack_dword(temp_rsp - 4, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_dword(temp_rsp - 8, cs_descriptor.dpl, EIP);
                temp_rsp -= 8;
            }
            else {
                write_new_stack_word(temp_rsp - 2, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_word(temp_rsp - 4, cs_descriptor.dpl, IP);
                temp_rsp -= 4;
            }

            branch_far(&cs_selector, &cs_descriptor, disp, CPL);
            RSP = temp_rsp;
        }
        else
#endif
        {
            Bit32u temp_RSP;

            if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
                temp_RSP = ESP;
            else
                temp_RSP = SP;

#if BX_SUPPORT_X86_64
            if (i->os64L()) {
                write_new_stack_qword(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 8, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_qword(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 16, cs_descriptor.dpl, RIP);
                temp_RSP -= 16;
            }
            else
#endif
            if (i->os32L()) {
                write_new_stack_dword(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 4, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_dword(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 8, cs_descriptor.dpl, EIP);
                temp_RSP -= 8;
            }
            else {
                write_new_stack_word(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 2, cs_descriptor.dpl,
                    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
                write_new_stack_word(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS],
                    temp_RSP - 4, cs_descriptor.dpl, IP);
                temp_RSP -= 4;
            }

            branch_far(&cs_selector, &cs_descriptor, disp, CPL);

            if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
                ESP = (Bit32u) temp_RSP;
            else
                SP = (Bit16u) temp_RSP;
        }

#if BX_SUPPORT_CET
        if (ShadowStackEnabled(CPL)) {
            call_far_shadow_stack_push(old_CS, temp_LIP, SSP);
        }
        track_indirect(CPL);
#endif

        return;
    }
    else {
        bx_descriptor_t gate_descriptor = cs_descriptor;
        bx_selector_t gate_selector = cs_selector;

        if (gate_descriptor.dpl < CPL) {
            exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
        }

        if (gate_descriptor.dpl < gate_selector.rpl) {
            exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
        }

#if BX_SUPPORT_X86_64
        if (long_mode()) {
            if (gate_descriptor.type != BX_386_CALL_GATE) {
                exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
            }
            if (!IS_PRESENT(gate_descriptor)) {
                exception(BX_NP_EXCEPTION, cs_raw & 0xfffc);
            }

            call_gate64(&gate_selector);
            return;
        }
#endif

        switch (gate_descriptor.type) {
        case BX_SYS_SEGMENT_AVAIL_286_TSS:
        case BX_SYS_SEGMENT_AVAIL_386_TSS:
            if (gate_descriptor.valid == 0 || gate_selector.ti) {
                exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
            }

            if (!IS_PRESENT(gate_descriptor)) {
                exception(BX_NP_EXCEPTION, cs_raw & 0xfffc);
            }

            task_switch(i, &gate_selector, &gate_descriptor,
                BX_TASK_FROM_CALL, dword1, dword2);
            return;

        case BX_TASK_GATE:
            task_gate(i, &gate_selector, &gate_descriptor, BX_TASK_FROM_CALL);
            return;

        case BX_286_CALL_GATE:
        case BX_386_CALL_GATE:
            if (!IS_PRESENT(gate_descriptor)) {
                exception(BX_NP_EXCEPTION, cs_raw & 0xfffc);
            }
            call_gate(&gate_descriptor);
            return;

        default:
            exception(BX_GP_EXCEPTION, cs_raw & 0xfffc);
        }
    }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::call_gate(bx_descriptor_t* gate_descriptor)
{
    bx_selector_t cs_selector;
    Bit32u dword1, dword2;
    bx_descriptor_t cs_descriptor;

    Bit16u dest_selector = gate_descriptor->u.gate.dest_selector;
    Bit32u new_EIP = gate_descriptor->u.gate.dest_offset;

    if ((dest_selector & 0xfffc) == 0) {
        exception(BX_GP_EXCEPTION, 0);
    }

    parse_selector(dest_selector, &cs_selector);
    fetch_raw_descriptor(&cs_selector, &dword1, &dword2, BX_GP_EXCEPTION);
    parse_descriptor(dword1, dword2, &cs_descriptor);

    if (cs_descriptor.valid == 0 || cs_descriptor.segment == 0 ||
        IS_DATA_SEGMENT(cs_descriptor.type) || cs_descriptor.dpl > CPL)
    {
        exception(BX_GP_EXCEPTION, dest_selector & 0xfffc);
    }

    if (!IS_PRESENT(cs_descriptor)) {
        exception(BX_NP_EXCEPTION, dest_selector & 0xfffc);
    }

    if (IS_CODE_SEGMENT_NON_CONFORMING(cs_descriptor.type) && (cs_descriptor.dpl < CPL))
    {
        Bit16u SS_for_cpl_x;
        Bit32u ESP_for_cpl_x;
        bx_selector_t ss_selector;
        bx_descriptor_t ss_descriptor;
        Bit16u return_SS, return_CS;
        Bit32u return_ESP, return_EIP;

        get_SS_ESP_from_TSS(cs_descriptor.dpl, &SS_for_cpl_x, &ESP_for_cpl_x);

        if ((SS_for_cpl_x & 0xfffc) == 0) {
            exception(BX_TS_EXCEPTION, 0);
        }

        parse_selector(SS_for_cpl_x, &ss_selector);
        fetch_raw_descriptor(&ss_selector, &dword1, &dword2, BX_TS_EXCEPTION);
        parse_descriptor(dword1, dword2, &ss_descriptor);

        if (ss_selector.rpl != cs_descriptor.dpl) {
            exception(BX_TS_EXCEPTION, SS_for_cpl_x & 0xfffc);
        }

        if (ss_descriptor.dpl != cs_descriptor.dpl) {
            exception(BX_TS_EXCEPTION, SS_for_cpl_x & 0xfffc);
        }

        if (ss_descriptor.valid == 0 || ss_descriptor.segment == 0 ||
            IS_CODE_SEGMENT(ss_descriptor.type) || !IS_DATA_SEGMENT_WRITEABLE(ss_descriptor.type))
        {
            exception(BX_TS_EXCEPTION, SS_for_cpl_x & 0xfffc);
        }

        if (!IS_PRESENT(ss_descriptor)) {
            exception(BX_SS_EXCEPTION, SS_for_cpl_x & 0xfffc);
        }

        unsigned param_count = gate_descriptor->u.gate.param_count & 0x1f;

        return_SS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value;
        if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
            return_ESP = ESP;
        else
            return_ESP = SP;

        return_CS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
        if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b)
            return_EIP = EIP;
        else
            return_EIP = IP;

        bx_segment_reg_t new_stack;
        new_stack.selector = ss_selector;
        new_stack.cache = ss_descriptor;
        new_stack.selector.rpl = cs_descriptor.dpl;
        new_stack.selector.value = (0xfffc & new_stack.selector.value) | new_stack.selector.rpl;

        if (ss_descriptor.u.segment.d_b) {
            Bit32u temp_ESP = ESP_for_cpl_x;

            if (gate_descriptor->type == BX_386_CALL_GATE) {
                write_new_stack_dword(&new_stack, temp_ESP - 4, cs_descriptor.dpl, return_SS);
                write_new_stack_dword(&new_stack, temp_ESP - 8, cs_descriptor.dpl, return_ESP);
                temp_ESP -= 8;

                for (unsigned n = param_count; n > 0; n--) {
                    temp_ESP -= 4;
                    Bit32u param = stack_read_dword(return_ESP + (n - 1) * 4);
                    write_new_stack_dword(&new_stack, temp_ESP, cs_descriptor.dpl, param);
                }

                write_new_stack_dword(&new_stack, temp_ESP - 4, cs_descriptor.dpl, return_CS);
                write_new_stack_dword(&new_stack, temp_ESP - 8, cs_descriptor.dpl, return_EIP);
                temp_ESP -= 8;
            }
            else {
                write_new_stack_word(&new_stack, temp_ESP - 2, cs_descriptor.dpl, return_SS);
                write_new_stack_word(&new_stack, temp_ESP - 4, cs_descriptor.dpl, (Bit16u) return_ESP);
                temp_ESP -= 4;

                for (unsigned n = param_count; n > 0; n--) {
                    temp_ESP -= 2;
                    Bit16u param = stack_read_word(return_ESP + (n - 1) * 2);
                    write_new_stack_word(&new_stack, temp_ESP, cs_descriptor.dpl, param);
                }

                write_new_stack_word(&new_stack, temp_ESP - 2, cs_descriptor.dpl, return_CS);
                write_new_stack_word(&new_stack, temp_ESP - 4, cs_descriptor.dpl, (Bit16u) return_EIP);
                temp_ESP -= 4;
            }

            ESP = temp_ESP;
        }
        else {
            Bit16u temp_SP = (Bit16u) ESP_for_cpl_x;

            if (gate_descriptor->type == BX_386_CALL_GATE) {
                write_new_stack_dword(&new_stack, (Bit16u)(temp_SP - 4), cs_descriptor.dpl, return_SS);
                write_new_stack_dword(&new_stack, (Bit16u)(temp_SP - 8), cs_descriptor.dpl, return_ESP);
                temp_SP -= 8;

                for (unsigned n = param_count; n > 0; n--) {
                    temp_SP -= 4;
                    Bit32u param = stack_read_dword(return_ESP + (n - 1) * 4);
                    write_new_stack_dword(&new_stack, temp_SP, cs_descriptor.dpl, param);
                }

                write_new_stack_dword(&new_stack, (Bit16u)(temp_SP - 4), cs_descriptor.dpl, return_CS);
                write_new_stack_dword(&new_stack, (Bit16u)(temp_SP - 8), cs_descriptor.dpl, return_EIP);
                temp_SP -= 8;
            }
            else {
                write_new_stack_word(&new_stack, (Bit16u)(temp_SP - 2), cs_descriptor.dpl, return_SS);
                write_new_stack_word(&new_stack, (Bit16u)(temp_SP - 4), cs_descriptor.dpl, (Bit16u) return_ESP);
                temp_SP -= 4;

                for (unsigned n = param_count; n > 0; n--) {
                    temp_SP -= 2;
                    Bit16u param = stack_read_word(return_ESP + (n - 1) * 2);
                    write_new_stack_word(&new_stack, temp_SP, cs_descriptor.dpl, param);
                }

                write_new_stack_word(&new_stack, (Bit16u)(temp_SP - 2), cs_descriptor.dpl, return_CS);
                write_new_stack_word(&new_stack, (Bit16u)(temp_SP - 4), cs_descriptor.dpl, (Bit16u) return_EIP);
                temp_SP -= 4;
            }

            SP = temp_SP;
        }

        if (new_EIP > cs_descriptor.u.segment.limit_scaled) {
            exception(BX_GP_EXCEPTION, 0);
        }

#if BX_SUPPORT_CET
        bx_address temp_LIP = get_laddr(BX_SEG_REG_CS, return_EIP);
        unsigned old_SS_DPL = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl;
        unsigned old_CPL = CPL;
#endif

        load_ss(&ss_selector, &ss_descriptor, cs_descriptor.dpl);
        load_cs(&cs_selector, &cs_descriptor, cs_descriptor.dpl);

        EIP = new_EIP;

#if BX_SUPPORT_CET
        if (ShadowStackEnabled(old_CPL)) {
            if (old_CPL == 3)
                BX_CPU_THIS_PTR msr.ia32_pl_ssp[3] = SSP;
        }

        if (ShadowStackEnabled(CPL)) {
            bx_address old_SSP = SSP;
            shadow_stack_switch(BX_CPU_THIS_PTR msr.ia32_pl_ssp[CPL]);
            if (old_SS_DPL != 3)
                call_far_shadow_stack_push(return_CS, temp_LIP, old_SSP);
        }
        track_indirect(CPL);
#endif
    }
    else
    {
        if (gate_descriptor->type == BX_386_CALL_GATE) {
            push_32(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
            push_32(EIP);
        }
        else {
            push_16(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
            push_16(IP);
        }

#if BX_SUPPORT_CET
        Bit32u temp_LIP = get_laddr(BX_SEG_REG_CS, ((gate_descriptor->type == BX_386_CALL_GATE) ? EIP : IP));
        Bit16u old_CS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
#endif

        branch_far(&cs_selector, &cs_descriptor, new_EIP, CPL);

#if BX_SUPPORT_CET
        if (ShadowStackEnabled(CPL)) {
            call_far_shadow_stack_push(old_CS, temp_LIP, SSP);
        }
        track_indirect(CPL);
#endif
    }
}

#if BX_SUPPORT_X86_64
void BX_CPP_AttrRegparmN(1) BX_CPU_C::call_gate64(bx_selector_t* gate_selector)
{
    bx_selector_t cs_selector;
    Bit32u dword1, dword2, dword3;
    bx_descriptor_t cs_descriptor;
    bx_descriptor_t gate_descriptor;

    fetch_raw_descriptor_64(gate_selector, &dword1, &dword2, &dword3, BX_GP_EXCEPTION);
    parse_descriptor(dword1, dword2, &gate_descriptor);

    Bit16u dest_selector = gate_descriptor.u.gate.dest_selector;
    if ((dest_selector & 0xfffc) == 0) {
        exception(BX_GP_EXCEPTION, 0);
    }

    parse_selector(dest_selector, &cs_selector);
    fetch_raw_descriptor(&cs_selector, &dword1, &dword2, BX_GP_EXCEPTION);
    parse_descriptor(dword1, dword2, &cs_descriptor);

    Bit64u new_RIP = GET64_FROM_HI32_LO32(dword3, gate_descriptor.u.gate.dest_offset);

    if (cs_descriptor.valid == 0 || cs_descriptor.segment == 0 ||
        IS_DATA_SEGMENT(cs_descriptor.type) || cs_descriptor.dpl > CPL)
    {
        exception(BX_GP_EXCEPTION, dest_selector & 0xfffc);
    }

    if (!IS_LONG64_SEGMENT(cs_descriptor) || cs_descriptor.u.segment.d_b)
    {
        exception(BX_GP_EXCEPTION, dest_selector & 0xfffc);
    }

    if (!IS_PRESENT(cs_descriptor)) {
        exception(BX_NP_EXCEPTION, dest_selector & 0xfffc);
    }

    Bit64u old_CS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
    Bit64u old_RIP = RIP;

#if BX_SUPPORT_CET
    bx_address temp_LIP = get_laddr(BX_SEG_REG_CS, RIP);
    unsigned old_SS_DPL = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl;
    unsigned old_CPL = CPL;
#endif

    if (IS_CODE_SEGMENT_NON_CONFORMING(cs_descriptor.type) && (cs_descriptor.dpl < CPL))
    {
        Bit64u RSP_for_cpl_x = get_RSP_from_TSS(cs_descriptor.dpl);
        Bit64u old_SS = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value;
        Bit64u old_RSP = RSP;

        write_new_stack_qword(RSP_for_cpl_x - 8, cs_descriptor.dpl, old_SS);
        write_new_stack_qword(RSP_for_cpl_x - 16, cs_descriptor.dpl, old_RSP);
        write_new_stack_qword(RSP_for_cpl_x - 24, cs_descriptor.dpl, old_CS);
        write_new_stack_qword(RSP_for_cpl_x - 32, cs_descriptor.dpl, old_RIP);
        RSP_for_cpl_x -= 32;

        branch_far(&cs_selector, &cs_descriptor, new_RIP, cs_descriptor.dpl);
        load_null_selector(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS], cs_descriptor.dpl);
        RSP = RSP_for_cpl_x;

#if BX_SUPPORT_CET
        if (ShadowStackEnabled(old_CPL)) {
            if (old_CPL == 3)
                BX_CPU_THIS_PTR msr.ia32_pl_ssp[3] = SSP;
        }
        if (ShadowStackEnabled(CPL)) {
            bx_address old_SSP = SSP;
            shadow_stack_switch(BX_CPU_THIS_PTR msr.ia32_pl_ssp[CPL]);
            if (old_SS_DPL != 3)
                call_far_shadow_stack_push(old_CS, temp_LIP, old_SSP);
        }
        track_indirect(CPL);
#endif
    }
    else
    {
        write_new_stack_qword(RSP - 8, CPL, old_CS);
        write_new_stack_qword(RSP - 16, CPL, old_RIP);

        branch_far(&cs_selector, &cs_descriptor, new_RIP, CPL);

        RSP -= 16;

#if BX_SUPPORT_CET
        if (ShadowStackEnabled(CPL)) {
            call_far_shadow_stack_push(old_CS, temp_LIP, SSP);
        }
        track_indirect(CPL);
#endif
    }
}
#endif

#if BX_SUPPORT_CET
void BX_CPP_AttrRegparmN(1) BX_CPU_C::shadow_stack_switch(bx_address new_SSP)
{
    SSP = new_SSP;

    if (SSP & 0x7) {
        //BX_ERROR(("shadow_stack_switch: SSP is not aligned to 8 byte boundary"));
        exception(BX_GP_EXCEPTION, 0);
    }
    if (!long64_mode() && GET32H(SSP) != 0) {
        //BX_ERROR(("shadow_stack_switch: 64-bit SSP not in 64-bit mode"));
        exception(BX_GP_EXCEPTION, 0);
    }
    if (!shadow_stack_atomic_set_busy(SSP, CPL)) {
        //BX_ERROR(("shadow_stack_switch: failure to set busy bit"));
        exception(BX_GP_EXCEPTION, 0);
    }
}

void BX_CPP_AttrRegparmN(3) BX_CPU_C::call_far_shadow_stack_push(Bit16u cs, bx_address lip, bx_address old_ssp)
{
#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        BX_CPU_THIS_PTR vmcs.shadow_stack_prematurely_busy = true;
#endif

    if (SSP & 0x7) {
        shadow_stack_write_dword(SSP - 4, CPL, 0);
        SSP &= ~BX_CONST64(0x7);
    }

    shadow_stack_push_64(cs);
    shadow_stack_push_64(lip);
    shadow_stack_push_64(old_ssp);

#if BX_SUPPORT_VMX
    if (BX_CPU_THIS_PTR in_vmx_guest)
        BX_CPU_THIS_PTR vmcs.shadow_stack_prematurely_busy = false;
#endif
}

#endif // BX_SUPPORT_CET
