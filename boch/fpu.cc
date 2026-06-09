#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "iodev.h"

#include "softfloat.h"

#define CHECK_PENDING_EXCEPTIONS 1

#if BX_SUPPORT_FPU
void BX_CPU_C::FPU_update_last_instruction(bxInstruction_c* i)
{
    // when FOPCODE deprecation is set, FOPCODE is updated only when unmasked x87 exception occurs
    if (!is_cpu_extension_supported(BX_ISA_FOPCODE_DEPRECATION))
        BX_CPU_THIS_PTR the_i387.foo = i->foo();

    BX_CPU_THIS_PTR the_i387.fcs = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
    BX_CPU_THIS_PTR the_i387.fip = BX_CPU_THIS_PTR prev_rip;

    // when FOPCODE deprecation is set, FCS/FDP are updated only when unmasked x87 exception occurs
    if (!is_cpu_extension_supported(BX_ISA_FDP_DEPRECATION)) {
        if (!i->modC0()) {
            BX_CPU_THIS_PTR the_i387.fds = BX_CPU_THIS_PTR sregs[i->seg()].selector.value;
            BX_CPU_THIS_PTR the_i387.fdp = RMAddr(i);
        }
    }
}

void BX_CPU_C::FPU_check_pending_exceptions(void)
{
    if (BX_CPU_THIS_PTR the_i387.get_partial_status() & FPU_SW_Summary)
    {
        // NE=1 selects the native or internal mode, which generates #MF,
        // which is an extension introduced with 80486.
        // NE=0 selects the original (backward compatible) FPU error
        // handling, which generates an IRQ 13 via the PIC chip.
#if BX_CPU_LEVEL >= 4
        if (BX_CPU_THIS_PTR cr0.get_NE() != 0) {
            exception(BX_MF_EXCEPTION, 0);
        }
        else
#endif
        {
            // MSDOS compatibility external interrupt (IRQ13)
            //BX_INFO(("math_abort: MSDOS compatibility FPU exception"));
            DEV_pic_raise_irq(13);
        }
    }
}

Bit16u BX_CPU_C::x87_get_FCS(void)
{
    if (is_cpu_extension_supported(BX_ISA_FCS_FDS_DEPRECATION))
        return 0;
    else
        return BX_CPU_THIS_PTR the_i387.fcs;
}

Bit16u BX_CPU_C::x87_get_FDS(void)
{
    if (is_cpu_extension_supported(BX_ISA_FCS_FDS_DEPRECATION))
        return 0;
    else
        return BX_CPU_THIS_PTR the_i387.fds;
}

bx_address BX_CPU_C::fpu_save_environment(bxInstruction_c* i)
{
    unsigned offset;

    /* read all registers in stack order and update x87 tag word */
    for (int n = 0; n < 8; n++) {
        // update tag only if it is not empty
        if (!IS_TAG_EMPTY(n)) {
            int tag = FPU_tagof(BX_READ_FPU_REG(n));
            BX_CPU_THIS_PTR the_i387.FPU_settagi(tag, n);
        }
    }

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    bx_address asize_mask = i->asize_mask();

    if (protected_mode())  /* Protected Mode */
    {
        if (i->os32L() || i->os64L())
        {
            Bit32u tmp;

            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_control_word();
            write_virtual_dword(i->seg(), eaddr, tmp);
            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_status_word();
            write_virtual_dword(i->seg(), (eaddr + 0x04) & asize_mask, tmp);
            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_tag_word();
            write_virtual_dword(i->seg(), (eaddr + 0x08) & asize_mask, tmp);
            tmp = (Bit32u)(BX_CPU_THIS_PTR the_i387.fip);
            write_virtual_dword(i->seg(), (eaddr + 0x0c) & asize_mask, tmp);
            tmp = x87_get_FCS() | ((Bit32u)(BX_CPU_THIS_PTR the_i387.foo)) << 16;
            write_virtual_dword(i->seg(), (eaddr + 0x10) & asize_mask, tmp);
            tmp = (Bit32u)(BX_CPU_THIS_PTR the_i387.fdp);
            write_virtual_dword(i->seg(), (eaddr + 0x14) & asize_mask, tmp);
            tmp = 0xffff0000 | x87_get_FDS();
            write_virtual_dword(i->seg(), (eaddr + 0x18) & asize_mask, tmp);

            offset = 0x1c;
        }
        else /* Protected Mode - 16 bit */
        {
            Bit16u tmp;

            tmp = BX_CPU_THIS_PTR the_i387.get_control_word();
            write_virtual_word(i->seg(), eaddr, tmp);
            tmp = BX_CPU_THIS_PTR the_i387.get_status_word();
            write_virtual_word(i->seg(), (eaddr + 0x02) & asize_mask, tmp);
            tmp = BX_CPU_THIS_PTR the_i387.get_tag_word();
            write_virtual_word(i->seg(), (eaddr + 0x04) & asize_mask, tmp);
            tmp = (Bit16u)(BX_CPU_THIS_PTR the_i387.fip) & 0xffff;
            write_virtual_word(i->seg(), (eaddr + 0x06) & asize_mask, tmp);
            tmp = x87_get_FCS();
            write_virtual_word(i->seg(), (eaddr + 0x08) & asize_mask, tmp);
            tmp = (Bit16u)(BX_CPU_THIS_PTR the_i387.fdp) & 0xffff;
            write_virtual_word(i->seg(), (eaddr + 0x0a) & asize_mask, tmp);
            tmp = x87_get_FDS();
            write_virtual_word(i->seg(), (eaddr + 0x0c) & asize_mask, tmp);

            offset = 0x0e;
        }
    }
    else   /* Real or V86 Mode */
    {
        Bit32u fp_ip = ((Bit32u)x87_get_FCS() << 4) +
            (Bit32u)(BX_CPU_THIS_PTR the_i387.fip);
        Bit32u fp_dp = ((Bit32u)x87_get_FDS() << 4) +
            (Bit32u)(BX_CPU_THIS_PTR the_i387.fdp);

        if (i->os32L())
        {
            Bit32u tmp;

            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_control_word();
            write_virtual_dword(i->seg(), eaddr, tmp);
            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_status_word();
            write_virtual_dword(i->seg(), (eaddr + 0x04) & asize_mask, tmp);
            tmp = 0xffff0000 | BX_CPU_THIS_PTR the_i387.get_tag_word();
            write_virtual_dword(i->seg(), (eaddr + 0x08) & asize_mask, tmp);
            tmp = 0xffff0000 | (fp_ip & 0xffff);
            write_virtual_dword(i->seg(), (eaddr + 0x0c) & asize_mask, tmp);
            tmp = ((fp_ip & 0xffff0000) >> 4) | BX_CPU_THIS_PTR the_i387.foo;
            write_virtual_dword(i->seg(), (eaddr + 0x10) & asize_mask, tmp);
            tmp = 0xffff0000 | (fp_dp & 0xffff);
            write_virtual_dword(i->seg(), (eaddr + 0x14) & asize_mask, tmp);
            tmp = (fp_dp & 0xffff0000) >> 4;
            write_virtual_dword(i->seg(), (eaddr + 0x18) & asize_mask, tmp);

            offset = 0x1c;
        }
        else  /* Real or V86 Mode - 16 bit */
        {
            Bit16u tmp;

            tmp = BX_CPU_THIS_PTR the_i387.get_control_word();
            write_virtual_word(i->seg(), eaddr, tmp);
            tmp = BX_CPU_THIS_PTR the_i387.get_status_word();
            write_virtual_word(i->seg(), (eaddr + 0x02) & asize_mask, tmp);
            tmp = BX_CPU_THIS_PTR the_i387.get_tag_word();
            write_virtual_word(i->seg(), (eaddr + 0x04) & asize_mask, tmp);
            tmp = fp_ip & 0xffff;
            write_virtual_word(i->seg(), (eaddr + 0x06) & asize_mask, tmp);
            tmp = (Bit16u)((fp_ip & 0xf0000) >> 4) | BX_CPU_THIS_PTR the_i387.foo;
            write_virtual_word(i->seg(), (eaddr + 0x08) & asize_mask, tmp);
            tmp = fp_dp & 0xffff;
            write_virtual_word(i->seg(), (eaddr + 0x0a) & asize_mask, tmp);
            tmp = (Bit16u)((fp_dp & 0xf0000) >> 4);
            write_virtual_word(i->seg(), (eaddr + 0x0c) & asize_mask, tmp);

            offset = 0x0e;
        }
    }

    return (eaddr + offset) & asize_mask;
}

bx_address BX_CPU_C::fpu_load_environment(bxInstruction_c* i)
{
    unsigned offset;

    bx_address eaddr = BX_CPU_RESOLVE_ADDR(i);

    bx_address asize_mask = i->asize_mask();

    if (protected_mode())  /* Protected Mode */
    {
        if (i->os32L() || i->os64L())
        {
            Bit32u tmp;

            tmp = read_virtual_dword(i->seg(), (eaddr + 0x18) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fds = tmp & 0xffff;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x14) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fdp = tmp;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x10) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fcs = tmp & 0xffff;
            BX_CPU_THIS_PTR the_i387.foo = (tmp >> 16) & 0x07ff;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x0c) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fip = tmp;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x08) & asize_mask);
            BX_CPU_THIS_PTR the_i387.twd = tmp & 0xffff;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x04) & asize_mask);
            BX_CPU_THIS_PTR the_i387.swd = tmp & 0xffff;
            BX_CPU_THIS_PTR the_i387.tos = (tmp >> 11) & 0x7;
            tmp = read_virtual_dword(i->seg(), eaddr);
            BX_CPU_THIS_PTR the_i387.cwd = tmp & 0xffff;
            offset = 0x1c;
        }
        else /* Protected Mode - 16 bit */
        {
            Bit16u tmp;

            tmp = read_virtual_word(i->seg(), (eaddr + 0x0c) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fds = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x0a) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fdp = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x08) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fcs = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x06) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fip = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x04) & asize_mask);
            BX_CPU_THIS_PTR the_i387.twd = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x02) & asize_mask);
            BX_CPU_THIS_PTR the_i387.swd = tmp;
            BX_CPU_THIS_PTR the_i387.tos = (tmp >> 11) & 0x7;
            tmp = read_virtual_word(i->seg(), eaddr);
            BX_CPU_THIS_PTR the_i387.cwd = tmp;
            /* opcode is defined to be zero */
            BX_CPU_THIS_PTR the_i387.foo = 0;
            offset = 0x0e;
        }
    }
    else   /* Real or V86 Mode */
    {
        Bit32u fp_ip, fp_dp;

        if (i->os32L())
        {
            Bit32u tmp;

            tmp = read_virtual_dword(i->seg(), (eaddr + 0x18) & asize_mask);
            fp_dp = (tmp & 0x0ffff000) << 4;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x14) & asize_mask);
            fp_dp |= tmp & 0xffff;
            BX_CPU_THIS_PTR the_i387.fdp = fp_dp;
            BX_CPU_THIS_PTR the_i387.fds = 0;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x10) & asize_mask);
            BX_CPU_THIS_PTR the_i387.foo = tmp & 0x07ff;
            fp_ip = (tmp & 0x0ffff000) << 4;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x0c) & asize_mask);
            fp_ip |= tmp & 0xffff;
            BX_CPU_THIS_PTR the_i387.fip = fp_ip;
            BX_CPU_THIS_PTR the_i387.fcs = 0;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x08) & asize_mask);
            BX_CPU_THIS_PTR the_i387.twd = tmp & 0xffff;
            tmp = read_virtual_dword(i->seg(), (eaddr + 0x04) & asize_mask);
            BX_CPU_THIS_PTR the_i387.swd = tmp & 0xffff;
            BX_CPU_THIS_PTR the_i387.tos = (tmp >> 11) & 0x7;
            tmp = read_virtual_dword(i->seg(), eaddr);
            BX_CPU_THIS_PTR the_i387.cwd = tmp & 0xffff;
            offset = 0x1c;
        }
        else  /* Real or V86 Mode - 16 bit */
        {
            Bit16u tmp;

            tmp = read_virtual_word(i->seg(), (eaddr + 0x0c) & asize_mask);
            fp_dp = (tmp & 0xf000) << 4;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x0a) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fdp = fp_dp | tmp;
            BX_CPU_THIS_PTR the_i387.fds = 0;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x08) & asize_mask);
            BX_CPU_THIS_PTR the_i387.foo = tmp & 0x07ff;
            fp_ip = (tmp & 0xf000) << 4;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x06) & asize_mask);
            BX_CPU_THIS_PTR the_i387.fip = fp_ip | tmp;
            BX_CPU_THIS_PTR the_i387.fcs = 0;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x04) & asize_mask);
            BX_CPU_THIS_PTR the_i387.twd = tmp;
            tmp = read_virtual_word(i->seg(), (eaddr + 0x02) & asize_mask);
            BX_CPU_THIS_PTR the_i387.swd = tmp;
            BX_CPU_THIS_PTR the_i387.tos = (tmp >> 11) & 0x7;
            tmp = read_virtual_word(i->seg(), eaddr);
            BX_CPU_THIS_PTR the_i387.cwd = tmp;
            offset = 0x0e;
        }
    }

    /* always set bit 6 as '1 */
    BX_CPU_THIS_PTR the_i387.cwd =
        (BX_CPU_THIS_PTR the_i387.cwd & ~FPU_CW_Reserved_Bits) | 0x0040;

    /* check for unmasked exceptions */
    if (FPU_PARTIAL_STATUS & ~FPU_CONTROL_WORD & FPU_CW_Exceptions_Mask)
    {
        /* set the B and ES bits in the status-word */
        FPU_PARTIAL_STATUS |= FPU_SW_Summary | FPU_SW_Backward;
    }
    else {
        /* clear the B and ES bits in the status-word */
        FPU_PARTIAL_STATUS &= ~(FPU_SW_Summary | FPU_SW_Backward);
    }

    return (eaddr + offset) & asize_mask;
}

#endif

#if BX_SUPPORT_FPU

#include <math.h>

void BX_CPU_C::print_state_FPU(void)
{
    static double scale_factor = pow(2.0, -63.0);
    static const char* cw_round_control[] = {
      "NEAREST", "DOWN", "UP", "CHOP"
    };
    static const char* cw_precision_control[] = {
      "32", "RES", "64", "80"
    };
    static const char* fp_class[] = {
      "ZERO", "SNAN", "QNAN", "-INF", "+INF", "DENORMAL", "NORMAL"
    };

    Bit32u reg;
    reg = BX_CPU_THIS_PTR the_i387.get_status_word();
    fprintf(stderr, "status  word: 0x%04x: ", reg);
    fprintf(stderr, "%s %s TOS%d %s %s %s %s %s %s %s %s %s %s %s\n",
        (reg & FPU_SW_Backward) ? "B" : "b",
        (reg & FPU_SW_C3) ? "C3" : "c3", (FPU_TOS & 7),
        (reg & FPU_SW_C2) ? "C2" : "c2",
        (reg & FPU_SW_C1) ? "C1" : "c1",
        (reg & FPU_SW_C0) ? "C0" : "c0",
        (reg & FPU_SW_Summary) ? "ES" : "es",
        (reg & FPU_SW_Stack_Fault) ? "SF" : "sf",
        (reg & FPU_SW_Precision) ? "PE" : "pe",
        (reg & FPU_SW_Underflow) ? "UE" : "ue",
        (reg & FPU_SW_Overflow) ? "OE" : "oe",
        (reg & FPU_SW_Zero_Div) ? "ZE" : "ze",
        (reg & FPU_SW_Denormal_Op) ? "DE" : "de",
        (reg & FPU_SW_Invalid) ? "IE" : "ie");

    reg = BX_CPU_THIS_PTR the_i387.get_control_word();
    fprintf(stderr, "control word: 0x%04x: ", reg);
    fprintf(stderr, "%s RC_%s PC_%s %s %s %s %s %s %s\n",
        (reg & FPU_CW_Inf) ? "INF" : "inf",
        (cw_round_control[(reg & FPU_CW_RC) >> 10]),
        (cw_precision_control[(reg & FPU_CW_PC) >> 8]),
        (reg & FPU_CW_Precision) ? "PM" : "pm",
        (reg & FPU_CW_Underflow) ? "UM" : "um",
        (reg & FPU_CW_Overflow) ? "OM" : "om",
        (reg & FPU_CW_Zero_Div) ? "ZM" : "zm",
        (reg & FPU_CW_Denormal) ? "DM" : "dm",
        (reg & FPU_CW_Invalid) ? "IM" : "im");

    reg = BX_CPU_THIS_PTR the_i387.get_tag_word();
    fprintf(stderr, "tag word:     0x%04x\n", reg);
    reg = BX_CPU_THIS_PTR the_i387.foo;
    fprintf(stderr, "operand:      0x%04x\n", reg);
    fprintf(stderr, "fip:          0x" FMT_ADDRX "\n",
        BX_CPU_THIS_PTR the_i387.fip);
    reg = BX_CPU_THIS_PTR the_i387.fcs;
    fprintf(stderr, "fcs:          0x%04x\n", reg);
    fprintf(stderr, "fdp:          0x" FMT_ADDRX "\n",
        BX_CPU_THIS_PTR the_i387.fdp);
    reg = BX_CPU_THIS_PTR the_i387.fds;
    fprintf(stderr, "fds:          0x%04x\n", reg);

    // print stack too
    int tos = FPU_TOS & 7;
    for (int i = 0; i < 8; i++) {
        const floatx80& fp = BX_FPU_REG(i);
        unsigned tag = BX_CPU_THIS_PTR the_i387.FPU_gettagi((i - tos) & 7);
        if (tag != FPU_Tag_Empty) tag = FPU_tagof(fp);
        double f = pow(2.0, ((0x7fff & fp.signExp) - 0x3fff));
        if (fp.signExp & 0x8000) f = -f;
#ifdef _MSC_VER
        f *= (double)(signed __int64)(fp.signif >> 1) * scale_factor * 2;
#else
        f *= fp.signif * scale_factor;
#endif
        softfloat_class_t f_class = extF80_class(fp);
        fprintf(stderr, "%sFP%d ST%d(%c):        raw 0x%04x:%08x%08x (%.10f) (%s)\n",
            i == tos ? "=>" : "  ", i, (i - tos) & 7,
            "v0se"[tag],
            fp.signExp & 0xffff, GET32H(fp.signif), GET32L(fp.signif),
            f, fp_class[f_class]);
    }
}

#include "softfloat-specialize.h"

/* -----------------------------------------------------------
 * Slimmed down version used to compile against a CPU simulator
 * rather than a kernel (ported by Kevin Lawton)
 * ------------------------------------------------------------ */

int FPU_tagof(const floatx80& reg)
{
    Bit32s exp = extF80_exp(reg);
    if (exp == 0)
    {
        if (!extF80_fraction(reg))
            return FPU_Tag_Zero;

        /* The number is a de-normal or pseudodenormal. */
        return FPU_Tag_Special;
    }

    if (exp == 0x7fff)
    {
        /* Is an Infinity, a NaN, or an unsupported data type. */
        return FPU_Tag_Special;
    }

    if (!(reg.signif & BX_CONST64(0x8000000000000000)))
    {
        /* Unsupported data type. */
        /* Valid numbers have the ms bit set to 1. */
        return FPU_Tag_Special;
    }

    return FPU_Tag_Valid;
}

#endif